#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "http/types.hpp"
#include "third_party/pgs_macros.h"
#include "third_party/pgs_log.h"

#ifndef PGS_REQUEST_BUFFER_SIZE
#   define PGS_REQUEST_BUFFER_SIZE PGS_KIB(8)
#endif

#ifndef PGS_RESPONSE_BODY_BUFFER_SIZE
#   define PGS_RESPONSE_BODY_BUFFER_SIZE PGS_KIB(4)
#endif

#ifndef PGS_RESPONSE_HEADER_BUFFER_SIZE
#   define PGS_RESPONSE_HEADER_BUFFER_SIZE 512
#endif


enum class Type {
    Accept,
    Read,
    Write,
    Close
};

struct Response {
    std::array<char, PGS_RESPONSE_HEADER_BUFFER_SIZE> header;
    std::array<char, PGS_RESPONSE_BODY_BUFFER_SIZE>   body;

    std::size_t header_len {0};
    std::size_t body_len   {0};

    std::string_view body_view {};

    iovec       iov[2];
    std::uint8_t iov_count {0};

    size_t get_body_len() {
        if (!body_view.empty())
            return body_view.size();
        else
            return body_len;
    }


    bool set_body_copy(std::string_view src) {
        if (src.size() > body.size())
            return false;

        memcpy(body.data(), src.data(), src.size());
        body_len = src.size();

        return true;
    }

    void set_body_static(std::string_view src) {
        body_view = src;
    }

    void prepare_iov(size_t offset) {
        if (header_len == 0)
            PGS_PANIC("Empty header");

        iov_count = 0;
        size_t skipped_bytes = 0;

        if (offset < header_len) {
            size_t header_offset = offset;
            size_t remaining_header = header_len - header_offset;

            iov[iov_count++] = {
                header.data() + header_offset,
                remaining_header
            };
        }

        skipped_bytes = header_len;

        size_t body_offset = offset > skipped_bytes ? offset - skipped_bytes : 0;

        if (!body_view.empty()) {
            if (body_offset < body_view.size())
                iov[iov_count++] = { const_cast<char *>(body_view.data()) + body_offset, body_view.size() - body_offset };
        }
        else if (body_len > 0)
            if (body_offset < body_len)
                iov[iov_count++] = { body.data() + body_offset, body_len - body_offset };
    }

    void reset() {
        header_len = 0;
        body_len   = 0;
        iov_count  = 0;
        body_view  = {};
    }
};

struct Request {
    static constexpr std::size_t    REQUEST_BUFFER_SIZE      {PGS_REQUEST_BUFFER_SIZE};
    static constexpr std::size_t    RESPONSE_BUFFER_SIZE     {PGS_RESPONSE_BODY_BUFFER_SIZE};


    Type                            type                     {};
    int                             fd                       {-1};

    sockaddr_storage                client_addr              {};
    socklen_t                       client_addr_len          {sizeof(client_addr)};

    std::array<char, REQUEST_BUFFER_SIZE>   buffer;
    Response                        response;
    size_t                          len                      {0};
    size_t                          off                      {0};

    size_t                          bytes_sent               {0};

    timespec start_time {};
    char endpoint[64] {};
    HttpMethod method {HttpMethod::Unknown};
    HttpVersion version {HttpVersion::Unknown};
    HttpStatus response_status {HttpStatus::Ok}; // filled by ResponseBuilder or route handler
    char user_agent[256]{};

#ifdef ENABLE_GEOIP
    char        country_code[3];
    char        subdivision_code[6];
#endif

    void reset() {
        type            = Type::Accept;
        fd              = -1;

        client_addr_len = sizeof(client_addr);

        len             = 0;
        off             = 0;

        bytes_sent      = 0;
        user_agent[0]   = '\0';
        response.reset();
    }
};
