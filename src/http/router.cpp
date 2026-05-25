#include "router.hpp"
#include "http/response.hpp"
#include "http/types.hpp"
#include "generated/pages.hpp"
#include "generated/assets.hpp"
#include <string_view>

#define PGS_LOG_TU_TAG "Router"
#define PGS_LOG_TU_ENABLED true
#include "third_party/pgs_log.h"


static void handle_page(Server &server, Request &req, const HttpRequestView &view) {
    for (const auto &p : generated_pages::pages) {
        if (view.target_path == p.route) {
            ResponseBuilder{HttpStatus::Ok}
                .content_type("text/html")
                .send_static(server, req, p.html);
            return;
        }

        if (view.target_path == p.fragment_route) {
            ResponseBuilder{HttpStatus::Ok}
                .content_type("text/html")
                .cache_control("public, max-age=31536000, immutable")
                .send_static(server, req, p.fragment());
            return;
        }
    }
}

static void handle_css(Server &server, Request &req, const HttpRequestView &view) {
    (void)view;
    ResponseBuilder{HttpStatus::Ok}
        .content_type("text/css")
        .cache_control("public, max-age=31536000, immutable")
        .send_static(server, req, generated_pages::css_content);
}

static void handle_asset(Server &server, Request &req, const HttpRequestView &view) {
    for (const auto &a : generated_assets::assets) {
        if (view.target_path == a.path) {
            ResponseBuilder{HttpStatus::Ok}
                .content_type(a.mime_type)
                .cache_control("public, max-age=31536000, immutable")
                .send_static(server, req,
                    std::string_view{reinterpret_cast<const char*>(a.data), a.size});
            return;
        }
    }
}

static constexpr std::string_view css_path { "/assets/styles." CSS_HASH_MACRO ".css"};

static constexpr Route routes[] = {
    PAGE_ROUTES(handle_page),
    {
        .method = HttpMethod::Get,
        .path = css_path,
        .handler = handle_css
    },
    ASSET_ROUTES(handle_asset)
};




const Route *router_match(const std::string_view &route, const HttpMethod &method) {
    PGS_LOG_DEBUG("Looking for route: %.*s", static_cast<int>(route.length()), route.data());
    for (const auto &r : routes) {
        if (r.method == method && r.path == route) {
            return &r;
        }
    }
    PGS_LOG_DEBUG("Didnt find route");
    return nullptr;
}
