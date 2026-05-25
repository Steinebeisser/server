#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#ifndef PGS_SERVER_NAME
#   define PGS_SERVER_NAME "SUPI DUPI STEIN SERVER"
#endif // PGS_SERVER_NAME

// ---------------------------------

#define HTTP_METHODS \
    X(Get,     "GET"     ) \
    X(Put,     "PUT"     ) \
    X(Post,    "POST"    ) \
    X(Head,    "HEAD"    ) \
    X(Trace,   "TRACE"   ) \
    X(Delete,  "DELETE"  ) \
    X(Options, "OPTIONS" ) \
    X(Connect, "CONNECT" )

enum class HttpMethod : uint8_t {
#define X(name, str) name,
    HTTP_METHODS
#undef X
    Unknown = 0xFF
};

static inline std::string_view method_to_string(HttpMethod m) {
    switch (m) {
#define X(name, str) \
        case HttpMethod::name: return str;
       HTTP_METHODS
#undef X
        case HttpMethod::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ---------------------------------

#define HTTP_VERSIONS \
    X(Http11, "HTTP/1.1")

enum class HttpVersion : uint8_t {
#define X(name, str) name,
    HTTP_VERSIONS
#undef X
    Unknown = 0xFF
};

static inline std::string_view version_to_string(HttpVersion v) {
    switch (v) {
#define X(name, str) \
        case HttpVersion::name: return str;
       HTTP_VERSIONS
#undef X
        case HttpVersion::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ---------------------------------

#define HTTP_STATUS_LIST \
    X(Ok, "OK", 200) \
    X(BadRequest, "Bad Request", 400) \
    X(NotFound, "Not Found", 404) \
    X(NotImplemented, "Not Implemented", 501) \

enum class HttpStatus {
#define X(enum_name, string, value) \
    enum_name = value,
    HTTP_STATUS_LIST
#undef X
};

static inline std::string_view http_status_to_string(HttpStatus status) {
    switch (status) {
#define X(name, str, value) \
        case HttpStatus::name: return str;
       HTTP_STATUS_LIST
#undef X
    }
    return "UNKNOWN";
}

static inline std::string_view http_status_to_digit_string(HttpStatus status) {
    switch (status) {
#define X(name, str, value) \
        case HttpStatus::name: return #value;
       HTTP_STATUS_LIST
#undef X
    }
    return "500";
}

// ---------------------------------

inline constexpr std::size_t MAX_REQUEST_HEADERS = 64;


struct HttpHeaderView {
    std::string_view name;
    std::string_view value;
};

struct HttpRequestView {
    HttpMethod method{HttpMethod::Unknown};
    std::string_view target_path{};
    HttpVersion version{HttpVersion::Unknown};

    HttpHeaderView headers[MAX_REQUEST_HEADERS]{};
    std::size_t headers_count{0};

    // void print() const {
    //     std::cout << "Method: " << method_to_string(method) << '\n' <<
    //                  "Target: `" << target_path << "`\n" <<
    //                  "Version: `" << version_to_string(version) << "`\n";
    //
    //     for (size_t i = 0; i < headers_count; ++i) {
    //         std::cout << "    " << headers[i].name << ": " << headers[i].value << '\n';
    //     }
    // }
};

#define KNOWN_RESPONSE_HEADER_LIST \
    X(content_type,     "Content-Type", "text/html") \
    X(server,           "Server",       PGS_SERVER_NAME) \
    X(connection,       "Connection",   "keep-alive") \
    X(cache_control,    "Cache-Control", "")

struct KnownResponseHeaders {
#define X(name, str, value) \
    std::string_view name = value;
    KNOWN_RESPONSE_HEADER_LIST
#undef X
};

struct HttpResponseView {
    HttpVersion          version                        {HttpVersion::Http11};
    HttpStatus           status                         {HttpStatus::Ok};

    KnownResponseHeaders known                          {};
    std::size_t          headers_count                  {0};
};
