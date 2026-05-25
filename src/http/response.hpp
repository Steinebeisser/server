#pragma once

#include "http/types.hpp"
#include "server/request.hpp"
#include "third_party/pgs_macros.h"
#include <string_view>

class Server;

class ResponseBuilder {
public:
    explicit ResponseBuilder(HttpStatus status) {
        view_.status = status;
    }

#define X(name, str, value) \
    ResponseBuilder& name (std::string_view v) { view_.known.name = v; return *this; }
    KNOWN_RESPONSE_HEADER_LIST
#undef X

    void send(Server &server, Request &req, const std::string_view &body);
    void send_static(Server &server, Request &req, const std::string_view &body);

private:
    HttpResponseView view_ {};
};
