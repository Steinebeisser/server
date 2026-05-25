#pragma once

#include <liburing.h>
#include "request.hpp"
#include "http/response.hpp"

class Server {
public:
    explicit Server(int server_fd);
    ~Server();

    void run();

    friend class ResponseBuilder;

private:
    static constexpr unsigned SQ_ENTRIES { 16384 };
    static constexpr unsigned MIN_SQE_SPACE_TO_CONTINUE { 16 };
    static constexpr std::size_t MAX_REQUESTS { 4096 };
    static constexpr __kernel_timespec KEEP_ALIVE_TIMEOUT {
        .tv_sec = 30,
        .tv_nsec = 0
    };

    int server_fd_;
    io_uring ring_ {};

    void submit_accept(Request *req);
    void submit_read  (Request *req, int client_fd);
    void submit_write (Request *req);
    void submit_close (Request *req);

    int handle_accept (Request *req, int result);
    int handle_read   (Request *req, int result);
    int handle_write  (Request *req, int result);
    int handle_close  (Request *req, int result);

    void handle_completion(io_uring_cqe *cqe, int &submissions);

    std::array<Request, MAX_REQUESTS> requests_;
    std::array<bool, MAX_REQUESTS> request_used_ {};
    Request* alloc_request();
    void free_request(Request* req);
};
