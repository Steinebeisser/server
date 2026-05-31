#pragma once

#include <liburing.h>
#include "request.hpp"
#include "http/response.hpp"

#ifdef ENABLE_STATS
#   include "stats/stat_container.hpp"
#endif

#ifdef ENABLE_GEOIP
#   include "third_party/IP2Location.h"
#endif


class Server {
public:
    explicit Server(int server_fd
#ifdef ENABLE_GEOIP
        , IP2Location *geoip
#endif
            );
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

#ifdef ENABLE_STATS
    StatContainer stats_;
#endif

#ifdef ENABLE_GEOIP
    IP2Location *geoip_ { nullptr };
#endif

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
