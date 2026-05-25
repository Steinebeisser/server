#include "server.hpp"
#include "http/parser.hpp"
#include "http/response.hpp"
#include "http/router.hpp"
#include "http/types.hpp"
#include "third_party/pgs_macros.h"
#include "server/request.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <string_view>
#include <unistd.h>
#include <variant>


#define PGS_LOG_TU_TAG "Server"
#define PGS_LOG_TU_ENABLED true
#include "third_party/pgs_log.h"

Request* Server::alloc_request() {
    for (std::size_t i = 0; i < requests_.size(); ++i) {
        if (!request_used_[i]) {
            request_used_[i] = true;
            requests_[i].reset();
            return &requests_[i];
        }
    }
    return nullptr;
}

void Server::free_request(Request* req) {
    std::ptrdiff_t diff = req - requests_.data();

    PGS_ASSERT(diff >= 0);
    PGS_ASSERT(static_cast<std::size_t>(diff) < requests_.size());

    std::size_t i = static_cast<std::size_t>(diff);
    request_used_[i] = false;
}


Server::Server(int server_fd)
    : server_fd_(server_fd)
{
    PGS_LOG_DEBUG("Creating Server");
    int res = io_uring_queue_init(SQ_ENTRIES, &ring_, 0);
    if (res < 0) {
        PGS_LOG_FATAL("Failed to init io_uring queue: %s", strerror(-res));
        exit(1);
    }
}

Server::~Server() {
    io_uring_queue_exit(&ring_);
}

void Server::run() {
    Request *accept_req { alloc_request() };
    if (!accept_req) {
        PGS_LOG_ERROR("No free request slot");
        return;
    }

    submit_accept(accept_req);

    int ret = io_uring_submit(&ring_);
    if (ret < 0) {
        PGS_LOG_ERROR("Initial io_uring_submit failed: %s", strerror(-ret));
        return;
    }

    while (true) {
        io_uring_cqe *cqe = nullptr;

        ret = io_uring_wait_cqe(&ring_, &cqe);
        if (ret < 0) {
            PGS_LOG_ERROR("io_uring_wait_cqe failed: %s", strerror(-ret));
            break;
        }

        int submissions {0};

        while (true) {
            handle_completion(cqe, submissions);
            io_uring_cqe_seen(&ring_, cqe);

            if (io_uring_sq_space_left(&ring_) < MIN_SQE_SPACE_TO_CONTINUE) {
                break;
            }

            ret = io_uring_peek_cqe(&ring_, &cqe);
            if (ret == -EAGAIN) {
                break;     // no remaining work in completion queue
            }
        }

        if (submissions > 0)
            io_uring_submit(&ring_);
    }
}

void Server::handle_completion(io_uring_cqe *cqe, int &submissions) {
    if (!cqe) return;

    Request *req = reinterpret_cast<Request*>(io_uring_cqe_get_data(cqe));
    if (!req) return;

    int res = cqe->res;

    switch (req->type) {
        case Type::Accept: submissions += handle_accept(req, res); break;
        case Type::Read:   submissions += handle_read  (req, res); break;
        case Type::Write:  submissions += handle_write (req, res); break;
        case Type::Close:  submissions += handle_close (req, res); break;
    }
}

void Server::submit_accept(Request *req) {
    req->type = Type::Accept;
    req->fd = server_fd_;
    req->client_addr_len = sizeof(req->client_addr);

    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        PGS_LOG_FATAL("[submit_accept] io_uring_get_sqe failed, no sqe available");
        exit(1);
    }

    io_uring_prep_accept(
        sqe,
        server_fd_,
        reinterpret_cast<struct sockaddr *>(&req->client_addr),
        &req->client_addr_len,
        0
    );

    io_uring_sqe_set_data(sqe, req);
}

void Server::submit_read(Request *req, int client_fd) {
    req->type = Type::Read;
    req->fd = client_fd;
    req->client_addr_len = sizeof(req->client_addr);

    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        PGS_LOG_ERROR("[submit_read] io_uring_get_sqe failed, no sqe available");
        return;
    }

    io_uring_prep_recv(
        sqe,
        client_fd,
        req->buffer.data(),
        req->buffer.size(),
        0
    );
    io_uring_sqe_set_flags(sqe, IOSQE_IO_LINK);
    io_uring_sqe_set_data(sqe, req);

    io_uring_sqe *timeout_sqe = io_uring_get_sqe(&ring_);
    if (!timeout_sqe) {
        PGS_LOG_ERROR("io_uring_get_sqe timeout sqe failed, no sqe available");
        return;
    }
    auto timeout = KEEP_ALIVE_TIMEOUT;
    io_uring_prep_link_timeout(timeout_sqe, &timeout, 0);
    io_uring_sqe_set_data(timeout_sqe, nullptr);

}

int Server::handle_accept(Request *req, int res) {
    submit_accept(req);
    int submitted = 1;

    if (res < 0) {
        PGS_LOG_ERROR("accept failed: %s", strerror(-res));
        return submitted;
    }

    Request *rreq = alloc_request();
    if (!rreq) {
        PGS_LOG_WARN("No free request slot for read");
        ::close(res);
        return submitted;
    }

    // io_uring_prep_accept(3) generates the installed file descriptor as its result.
    submit_read(rreq, res);
    return submitted + 1;
}

static constexpr HttpRequestView not_found_view {
        .method = HttpMethod::Get,
        .target_path = "/404/",
        .version =  HttpVersion::Http11 ,
        .headers = {},
        .headers_count = 0
};
static constexpr std::string_view NOT_FOUND_FALLBACK = { "NOT FOUND" };

int Server::handle_read(Request *req, int res) {
    if (res < 0) {
        if (-res != ECONNRESET && -res != ENOTCONN && -res != EPIPE && -res != -ECANCELED) {
            PGS_LOG_ERROR("recv failed: %s", strerror(-res));
        }
        submit_close(req);
        return 1;
    }

    if (res == 0) {
        submit_close(req);
        return 1;
    }

    req->len = static_cast<std::size_t>(res);
    auto http_res = parse_http(*req);
    if (!http_res) {
        static constexpr std::string_view ERROR_BODY = { "Bad Request" };
        ResponseBuilder{http_res.error().status}
            .send(*this, *req, ERROR_BODY);
        return 1;
    }


    if (std::holds_alternative<HttpRequestView>(http_res.value())) {
        HttpRequestView view = get<HttpRequestView>(http_res.value());

        const Route* r = router_match(view.target_path, view.method);
        if (r == nullptr) {
            const Route* not_found = router_match(not_found_view.target_path, HttpMethod::Get);
            if (not_found == nullptr) {
                ResponseBuilder{HttpStatus::NotFound}
                    .send_static(*this, *req, NOT_FOUND_FALLBACK);
            } else {
                PGS_LOG_DEBUG("NICHT FUNDI SENDI MÜSSI");
                not_found->handler(*this, *req, not_found_view);
            }
            return 1;
        } else {
            r->handler(*this, *req, view);
        }
        return 1;
    } else {
        PGS_LOG_DEBUG("Need to read more");
    }
    return 1;
}

int Server::handle_write(Request *req, int res) {
    if (res < 0) {
        PGS_LOG_ERROR("send failed: %s", strerror(-res));
        submit_close(req);
        return 0;
    }

    req->bytes_sent += static_cast<size_t>(res);
    if (req->bytes_sent < req->response.get_body_len()) {
        submit_write(req);
        return 1;
    }

    int fd = req->fd;
    req->reset();
    submit_read(req, fd);

    return 1;
}

int Server::handle_close(Request *req, int res) {
    if (res < 0) {
        PGS_LOG_ERROR("close failed: %s", strerror(-res));
    }

    free_request(req);
    return 0;
}


void Server::submit_close(Request *req) {
    req->type = Type::Close;

    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        PGS_LOG_ERROR("io_uring_get_sqe failed for close");
        ::close(req->fd);
        free_request(req);
        return;
    }

    io_uring_prep_close(sqe, req->fd);
    io_uring_sqe_set_data(sqe, req);
}

void Server::submit_write(Request *req) {
    req->type = Type::Write;

    io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
        PGS_LOG_ERROR("io_uring_get_sqe failed for write");
        submit_close(req);
        return;
    }

    req->response.prepare_iov(req->bytes_sent);

    io_uring_prep_writev(
        sqe,
        req->fd,
        req->response.iov,
        req->response.iov_count,
        0
    );

    io_uring_sqe_set_data(sqe, req);
}
