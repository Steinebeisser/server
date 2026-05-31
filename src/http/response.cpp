#include "response.hpp"
#include <charconv>
#include <cstdio>
#include <cstring>
#include <string_view>
#include "http/types.hpp"
#include "server/request.hpp"
#include "server/server.hpp"

static inline bool append_raw(char *dst, size_t max_size, size_t &pos, std::string_view s) {
    if (s.size() > max_size - pos)
        return false;

    memcpy(dst + pos, s.data(), s.size());
    pos += s.size();
    return true;
}

static inline bool append_header_if(char *out, size_t max, size_t &pos,
                                    std::string_view name, std::string_view value)
{
    if (value.empty()) return true;
    return append_raw(out, max, pos, name)   &&
           append_raw(out, max, pos, ": ")   &&
           append_raw(out, max, pos, value)  &&
           append_raw(out, max, pos, "\r\n");
}

PGS_WARN_UNUSED_RESULT bool build_response_header(Request &req, const HttpResponseView &view) {
    char                   *out         { req.response.header.data() };
    size_t                 max_size     { req.response.header.size() };
    size_t                 pos          { 0 };

    if (!append_raw(out, max_size, pos, version_to_string(view.version))) return false;
    if (!append_raw(out, max_size, pos, " ")) return false;
    if (!append_raw(out, max_size, pos, http_status_to_digit_string(view.status))) return false;
    if (!append_raw(out, max_size, pos, " ")) return false;
    if (!append_raw(out, max_size, pos, "\r\n")) return false;

    if (req.response.get_body_len() > 0) {
        char cl[20]{};
        std::to_chars(cl, cl + sizeof(cl), req.response.get_body_len());

        if (!append_raw(out, max_size, pos, "Content-Length: ")) return false;
        if (!append_raw(out, max_size, pos, cl )) return false;
        if (!append_raw(out, max_size, pos, "\r\n")) return false;
    }

#define X(name, str, value) \
    if (!append_header_if(out, max_size, pos, str, view.known.name)) return false;
    KNOWN_RESPONSE_HEADER_LIST
#undef X


    if (!append_raw(out, max_size, pos, "\r\n")) return false;
    req.response.header_len = pos;

    return true;
}

void ResponseBuilder::send(Server &server, Request &req, const std::string_view &body) {
    req.response_status = view_.status;
    if (!req.response.set_body_copy(body) || !build_response_header(req, view_))
        server.submit_close(&req);
    else
        server.submit_write(&req);
}

void ResponseBuilder::send_static(Server &server, Request &req, const std::string_view &body) {
    req.response_status = view_.status;
    req.response.set_body_static(body);
    if (!build_response_header(req, view_))
        server.submit_close(&req);
    else
        server.submit_write(&req);
}
