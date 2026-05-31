#include "server/server.hpp"
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <liburing/io_uring.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>

#define PGS_MACROS_IMPLEMENTATION
#include "third_party/pgs_macros.h"

#define PGS_LOG_TU_TAG "Main"
#define PGS_LOG_TU_ENABLED true
#define PGS_LOG_IMPLEMENTATION

#include "third_party/pgs_log.h"

#ifdef ENABLE_GEOIP
#   include "third_party/IP2Location.h"
#endif

#define PORT_MAX 65535
// CQ will have double the entries
//   By default, the CQ ring will have twice the number of entries as specified by entries for the SQ ring.


#define PGS_ARGS \
    PGS_ARG(PGS_ARG_VALUE,      port, 'p', "port", "Provide the port the server should run on", PGS_ARGS_UINT) \
    PGS_ARG(PGS_ARG_OPTIONAL, ip,   'i', "ip",   "Provide the IP the server should run on, if none is given defaults to ::\n" \
        "if a specific ip address is given it goes to either ipv4 or ipv6 only\n" \
        " - ::          dual stack (default)\n" \
        " - ::1         ipv6 loopback\n" \
        " - 0.0.0.0     ipv4 all interfaces\n" \
        " - 192.168.1.5 ipv4 specific interface\n" \
        " - 2001:db8::1 ipv6 specific interface", NULL) \
    PGS_ARG(PGS_ARG_OPTIONAL,   help, 'h', "help", "Show help message", NULL)

#define PGS_ARGS_IMPLEMENTATION
#include "third_party/pgs_args.h"

static int detect_af(const char *ip) {
    if (!ip) return -1;

    in_addr  dummy4 {};
    in6_addr dummy6 {};

    if (inet_pton(AF_INET,  ip, &dummy4) == 1) return AF_INET;
    if (inet_pton(AF_INET6, ip, &dummy6) == 1) return AF_INET6;

    return -1;
}

int main(int argc, char **argv) {
    pgs_args_t args {};
    if (!pgs_args_parse(&args, argc, argv, false)) {
        PGS_LOG_ERROR("Failed to parse args");
        return 1;
    }

#ifdef ENABLE_GEOIP
    PGS_LOG_ERROR("HAVE GEO IP TRACKING ENABLED");
    IP2Location *IP2LocationObj = IP2Location_open(const_cast<char*>("third_party/ip2location-db-bin/IP2LOCATION-LITE-DB3.IPV6.BIN"));

    printf("IP2Location API version: %s \n", IP2Location_api_version_string());

    if (IP2LocationObj == NULL)
    {
        printf("Please install the database in correct path.\n");
        return -1;
    }

    fprintf(stdout,"IP2Location BIN version: %s\n", IP2Location_bin_version(IP2LocationObj));

    IP2Location_open_mem(IP2LocationObj, IP2LOCATION_SHARED_MEMORY);
#endif


    if (args.help_present) {
        if (args.help_value) {
            pgs_args_print_help_specific_name(args.help_value);
        } else {
            pgs_args_print_help();
        }
        return 0;
    }

    if (!args.port_present) {
        PGS_LOG_ERROR("Usage: %s -p [port]", argv[0]);
        return 1;
    }

    const char *bind_ip = args.ip_present ? args.ip_value : "::";
    int af = detect_af(bind_ip);
    if (af == -1) {
        PGS_LOG_ERROR("Cannot determine address family for IP: `%s`, not a valid IPv4 or IPv6 address", bind_ip);
        return 1;
    }
    bool dual_stack = af == AF_INET6 && (strcmp(bind_ip, "::") == 0);

    uint64_t port { pgs_args_parse_count(args.port_value) };
    if (port > PORT_MAX) {
        PGS_LOG_ERROR("The Port cannot be higher than %u", static_cast<unsigned>(PORT_MAX));
        return 1;
    }

    int server_fd = socket(af, SOCK_STREAM, 0);
    if (server_fd < 0 ) {
        PGS_LOG_ERROR("Failed to create socket: %s", strerror(errno));
        return 1;
    }

    int opt { 1 };
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        PGS_LOG_ERROR("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        return 1;
    }

    if (af == AF_INET6) {
        int v6 = dual_stack ? 0 : 1;
        if (setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6, sizeof(v6)) < 0) {
            PGS_LOG_ERROR("setsockopt(IPV6_V6ONLY) failed: %s", strerror(errno));
            return 1;
        }
    }

    sockaddr_storage addr {};
    socklen_t addr_len = 0;

    if (af == AF_INET) {
        sockaddr_in *addr4 = reinterpret_cast<sockaddr_in*>(&addr);
        addr4->sin_family = AF_INET;
        addr4->sin_port   = htons(static_cast<uint16_t>(port));

        int ret = inet_pton(AF_INET, bind_ip, &addr4->sin_addr);
        if (ret <= 0) {
            PGS_LOG_ERROR("Invalid IPv4 address: %s", bind_ip);
            if (ret == -1) {
                PGS_LOG_ERROR("%s", strerror(errno));
            }
            return 1;
        }
        addr_len = sizeof(sockaddr_in);
    } else if (af == AF_INET6) {
        sockaddr_in6 *addr6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port   = htons(static_cast<uint16_t>(port));

        int ret = inet_pton(AF_INET6, bind_ip, &addr6->sin6_addr);
        if (ret <= 0) {
            PGS_LOG_ERROR("Invalid IPv6 address: %s", bind_ip);
            if (ret == -1) {
                PGS_LOG_ERROR("%s", strerror(errno));
            }
            return 1;
        }

        addr_len = sizeof(sockaddr_in6);
    } else {
        PGS_LOG_ERROR("Invalid AF type");
        return 1;
    }


    if(bind(server_fd, reinterpret_cast<sockaddr*>(&addr), addr_len) < 0) {
        PGS_LOG_ERROR("Failed to bind to Port: %u: %s", static_cast<unsigned>(port), strerror(errno));
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        PGS_LOG_ERROR("Failed to listen: %s", strerror(errno));
        return 1;
    }

    PGS_LOG_INFO("Listening on %s%s%s:%lu%s",
        af == AF_INET6 ? "[" : "",
        bind_ip,
        af == AF_INET6 ? "]" : "",
        port,
        dual_stack ? "  (dual-stack IPv4+IPv6)" : ""
    );

    static Server server(server_fd
#ifdef ENABLE_GEOIP
            , IP2LocationObj
#endif
            );

    server.run();
}


