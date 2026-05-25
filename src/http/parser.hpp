#pragma once

#include "server/request.hpp"
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <string_view>
#include <variant>
#include "third_party/pgs_macros.h"
#include "types.hpp"

PGS_STATIC_ASSERT(std::endian::native == std::endian::little, "Parser assumes little-endian native loads");


static constexpr uint32_t u32(const char *s) {
    return static_cast<uint32_t>(static_cast<uint8_t>(s[0]))       |
           static_cast<uint32_t>(static_cast<uint8_t>(s[1])) << 8  |
           static_cast<uint32_t>(static_cast<uint8_t>(s[2])) << 16 |
           static_cast<uint32_t>(static_cast<uint8_t>(s[3])) << 24;
}

static constexpr uint64_t u64(const char *s) {
    return static_cast<uint64_t>(static_cast<uint8_t>(s[0]))        |
           static_cast<uint64_t>(static_cast<uint8_t>(s[1])) << 8   |
           static_cast<uint64_t>(static_cast<uint8_t>(s[2])) << 16  |
           static_cast<uint64_t>(static_cast<uint8_t>(s[3])) << 24  |
           static_cast<uint64_t>(static_cast<uint8_t>(s[4])) << 32  |
           static_cast<uint64_t>(static_cast<uint8_t>(s[5])) << 40  |
           static_cast<uint64_t>(static_cast<uint8_t>(s[6])) << 48  |
           static_cast<uint64_t>(static_cast<uint8_t>(s[7])) << 56;
}

static constexpr uint32_t K_GET  = u32("GET ");
static constexpr uint32_t K_PUT  = u32("PUT ");

static constexpr uint64_t MASK4     = 0x00000000FFFFFFFFull;
static constexpr uint64_t MASK6     = 0x0000FFFFFFFFFFFFull;
static constexpr uint64_t MASK7     = 0x00FFFFFFFFFFFFFFull;
static constexpr uint32_t K_POST    = u64("POST \0\0\0") & MASK4;
static constexpr uint32_t K_HEAD    = u64("HEAD \0\0\0") & MASK4;
static constexpr uint64_t K_TRACE   = u64("TRACE \0\0")  & MASK6;
static constexpr uint64_t K_DELETE  = u64("DELETE \0")   & MASK7;
static constexpr uint64_t K_OPTIONS = u64("OPTIONS ");
static constexpr uint64_t K_CONNECT = u64("CONNECT ");


static inline HttpMethod parse_method(std::string_view buf, int &out_len) {
    uint32_t v4 {};
    if (buf.size() < 4)
        return HttpMethod::Unknown;

    memcpy(&v4, buf.data(), 4);
    switch (v4) {
        case K_GET:  out_len = 3; return HttpMethod::Get;
        case K_PUT:  out_len = 3; return HttpMethod::Put;
        default: break;
    }

    if (buf.size() < 8)
        return HttpMethod::Unknown;

    uint64_t v8 {};
    memcpy(&v8, buf.data(), 8);
    if ((v8 & MASK4) == K_POST)    { out_len = 4; return HttpMethod::Post;    }
    if ((v8 & MASK4) == K_HEAD)    { out_len = 4; return HttpMethod::Head;    }

    if ((v8 & MASK6) == K_TRACE)   { out_len = 5; return HttpMethod::Trace;   }
    if ((v8 & MASK7) == K_DELETE)  { out_len = 6; return HttpMethod::Delete;  }
    if ( v8          == K_OPTIONS) { out_len = 7; return HttpMethod::Options; }
    if ( v8          == K_CONNECT) { out_len = 7; return HttpMethod::Connect; }

    return HttpMethod::Unknown;
}

static constexpr uint64_t K_HTTP11 = u64("HTTP/1.1");

static inline HttpVersion parse_version(std::string_view buf, int &out_len) {
    if (buf.size() < 8)
        return HttpVersion::Unknown;

    uint64_t v8 {};
    memcpy(&v8, buf.data(), 8);
    if (v8 == K_HTTP11) { out_len = 8; return HttpVersion::Http11; }
    return HttpVersion::Unknown;
}



enum class ParseResult {
    Invalid,
    Incomplete,
    Valid
};

struct NeedMore {
    std::size_t bytes_read {};
};

struct HttpParseError {
    HttpStatus status;
    std::string_view message;
};

using HttpParseSuccess = std::variant<NeedMore, HttpRequestView>;
using HttpParseResult = std::expected<HttpParseSuccess, HttpParseError>;

HttpParseResult parse_http(Request& req);
