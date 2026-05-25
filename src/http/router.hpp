#pragma once

#include "http/types.hpp"
#include "server/request.hpp"

class Server;

struct Route {
    HttpMethod       method;
    std::string_view path;
    void (*handler)(Server &server, Request &req, const HttpRequestView &view);
};


const Route *router_match(const std::string_view &route, const HttpMethod &method);
