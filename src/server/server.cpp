#include "server.hpp"
#include "http/parser.hpp"
#include "http/response.hpp"
#include "http/router.hpp"
#include "http/types.hpp"
#include "stats/stat_container.hpp"
#include "stats/stats.hpp"
#include "third_party/pgs_macros.h"
#include "server/request.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <string_view>
#include <unistd.h>
#include <variant>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "stats/location.hpp"

#ifndef PGS_DB_NAME
#define PGS_DB_NAME "server_stats.db"
#endif

#define PGS_LOG_TU_TAG "Server"
#define PGS_LOG_TU_ENABLED true
#include "third_party/pgs_log.h"

#ifdef ENABLE_GEOIP

static char* write_u8(char* p, uint8_t n) {
    if (n >= 100) { *p++ = static_cast<char>('0' + n / 100); n %= 100; *p++ = static_cast<char>('0' + n / 10); }
    else if (n >= 10) { *p++ = static_cast<char>('0' + n / 10); }
    *p++ = static_cast<char>('0' + n % 10);
    return p;
}

static void sockaddr_to_str(const sockaddr_storage &addr, char *buf, std::size_t len) {
    if (addr.ss_family == AF_INET) {
        const auto *a4 = reinterpret_cast<const sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &a4->sin_addr, buf, static_cast<socklen_t>(len));
        return;
    }

    const auto *a6 = reinterpret_cast<const sockaddr_in6*>(&addr);
    if (IN6_IS_ADDR_V4MAPPED(&a6->sin6_addr)) {
        const uint8_t *b = a6->sin6_addr.s6_addr + 12;
        char *p = buf;
        p = write_u8(p, b[0]); *p++ = '.';
        p = write_u8(p, b[1]); *p++ = '.';
        p = write_u8(p, b[2]); *p++ = '.';
        p = write_u8(p, b[3]);
        *p = '\0';
    } else {
        inet_ntop(AF_INET6, &a6->sin6_addr, buf, static_cast<socklen_t>(len));
    }
}

#endif // ENABLE_GEOIP

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


Server::Server(int server_fd
#ifdef ENABLE_GEOIP
        , IP2Location *geoip
#endif
        )
    : server_fd_(server_fd)
#ifdef ENABLE_GEOIP
        , geoip_(geoip)
#endif
{
    PGS_LOG_DEBUG("Creating Server");
    int res = io_uring_queue_init(SQ_ENTRIES, &ring_, 0);
    if (res < 0) {
        PGS_LOG_FATAL("Failed to init io_uring queue: %s", strerror(-res));
        exit(1);
    }

#ifdef ENABLE_STATS
    if (!stats_.init(PGS_DB_NAME)) {
#ifdef EXIT_IF_STATS_FAIL_INITIALIZING
        exit(1);
#endif
    }
#endif
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
                break;
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

    clock_gettime(CLOCK_MONOTONIC, &req->start_time);

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
    io_uring_prep_link_timeout(timeout_sqe, const_cast<__kernel_timespec*>(&KEEP_ALIVE_TIMEOUT), 0);
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

#ifdef ENABLE_GEOIP
    if (geoip_) {
        char ip_str[INET6_ADDRSTRLEN] {};
        sockaddr_to_str(req->client_addr, ip_str, sizeof(ip_str));


        IP2LocationRecord *geo = IP2Location_get_all(geoip_, const_cast<char*>(ip_str));
        if (geo) {
            PGS_LOG_DEBUG("Connection from %s (%s, %s, %s)",
                ip_str,
                geo->country_short,
                geo->region,
                geo->city
            );

            if (geo->country_short) {
                strncpy(rreq->country_code, geo->country_short, 2);
                rreq->country_code[2] = '\0';
            } else {
                rreq->country_code[0] = '\0';
            }

            const char* sub_code = LocationMapper::get_subdivision_code_by_name(geo->region);
            if (sub_code) {
                strncpy(rreq->subdivision_code, sub_code, sizeof(rreq->subdivision_code)-1);
                rreq->subdivision_code[sizeof(rreq->subdivision_code)-1] = '\0';
            } else {
                rreq->subdivision_code[0] = '\0';
            }
            IP2Location_free_record(geo);
        }
    }
#endif

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

int Server::handle_read(Request *req, int res) {
    if (res < 0) {
        if (-res != ECONNRESET && -res != ENOTCONN && -res != EPIPE && -res != ECANCELED) {
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
        const auto& path = view.target_path;
        size_t len = std::min(path.size(), sizeof(req->endpoint) - 1);
        memcpy(req->endpoint, path.data(), len);
        req->endpoint[len] = '\0';
        req->method = view.method;
        req->version = view.version;

        const Route* r = router_match(view.target_path, view.method);
        if (r == nullptr) {
            handle_not_found(*this, *req, view);
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

#ifdef ENABLE_STATS
    timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    time_t sec_diff = end_time.tv_sec - req->start_time.tv_sec;
    long nsec_diff = end_time.tv_nsec - req->start_time.tv_nsec;

    auto latency_ns = (sec_diff) * 1'000'000'000LL + nsec_diff;
    auto latency_ms = latency_ns / 1'000'000;

    StatRecord *sr = stats_.get();
    *sr = {
        .timestamp = time(nullptr),

        .endpoint = {},
        .method = req->method,
        .latency_ns = latency_ns,
        .request_bytes =  static_cast<int64_t>(req->len),
        .response_bytes = static_cast<int64_t>(req->bytes_sent),
        .status_code = req->response_status,
        .version = req->version,

#ifdef ENABLE_GEOIP
        .country_code = {},
        .subdivision_code = {},
#endif
    };
#ifdef ENABLE_GEOIP
    memcpy(sr->country_code, req->country_code, sizeof(sr->country_code));
    memcpy(sr->subdivision_code, req->subdivision_code, sizeof(sr->subdivision_code));
#endif

    size_t len = 0;
    while (len < strlen(req->endpoint) && req->endpoint[len] != '\0') ++len;
    size_t copy_len = std::min(len, sizeof(sr->endpoint) - 1);
    memcpy(sr->endpoint, req->endpoint, copy_len);
    sr->endpoint[copy_len] = '\0';

    stats_.maybe_flush();


#ifdef ENABLE_GEOIP
    PGS_LOG_DEBUG("TS: %zu\n" "Endpoint %s\n" "Methods: %s\n" "Latency NS: %ld (%lld ms)\n" "REquest Bytes: %zu\n" "Response bytes: %zu\n" "Stats Code: %s\n" "Country: %s\n" "Subdivision: %s\n" "Version: %s\n", sr->timestamp, sr->endpoint, method_to_string(sr->method).data(), sr->latency_ns, latency_ms, sr->request_bytes, sr->response_bytes, http_status_to_string(sr->status_code).data(), sr->country_code, sr->subdivision_code, version_to_string(sr->version).data());
#else
    PGS_LOG_DEBUG("TS: %zu\n" "Endpoint %s\n" "Methods: %s\n" "Latency NS: %ld (%lld ms)\n" "REquest Bytes: %zu\n" "Response bytes: %zu\n" "Stats Code: %s\n" "Version: %s\n", sr->timestamp, sr->endpoint, method_to_string(sr->method).data(), sr->latency_ns, latency_ms, sr->request_bytes, sr->response_bytes, http_status_to_string(sr->status_code).data(), version_to_string(sr->version).data());
#endif

#endif // ENABLE_STATS

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
