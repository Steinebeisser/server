#include "parser.hpp"
#include "server/request.hpp"
#include <cstddef>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <string_view>

static void chomp_char(std::string_view &buf, char c) {
    while (!buf.empty() && buf.front() == c)
        buf.remove_prefix(1);
}

/* HTTP-message   = start-line CRLF
                   *( field-line CRLF )
                   CRLF
                   [ message-body ]

 * start-line     = request-line / status-line

 * request-line   = method SP request-target SP HTTP-version

 * status-line = HTTP-version SP status-code SP [ reason-phrase ]
*/

HttpParseResult parse_http(Request &req) {
    std::string_view buf(req.buffer.data(), req.len);

    int method_len = 0;
    HttpMethod method = parse_method(buf, method_len);
    if (method == HttpMethod::Unknown) {
        return std::unexpected(
            HttpParseError{
                .status = HttpStatus::NotImplemented,
                .message = "Not Implemeted"
            }
        );
    }

    HttpRequestView view {};

    view.method = method;

    buf.remove_prefix(static_cast<size_t>(method_len));
    chomp_char(buf, ' ');


    size_t end_of_target = buf.find_first_of(' ');
    if (end_of_target == std::string_view::npos) {
        return std::unexpected(
            HttpParseError{
                .status = HttpStatus::NotImplemented,
                .message = "Not Implemeted"
            }
        );
    }
    auto target = buf.substr(0, end_of_target);
    buf.remove_prefix(end_of_target);
    view.target_path = target;

    chomp_char(buf, ' ');
    int version_len = 0;
    HttpVersion version = parse_version(buf, version_len);
    if (version == HttpVersion::Unknown) {
        return std::unexpected(
            HttpParseError{
                .status = HttpStatus::BadRequest,
                .message = "Bad Request"
            }
        );
    }

    view.version = version;

    buf.remove_prefix(static_cast<size_t>(version_len));
    if (!buf.starts_with("\r\n")) {
        return std::unexpected(
            HttpParseError{
                .status = HttpStatus::BadRequest,
                .message = "Bad Request"
            }
        );
    }
    buf.remove_prefix(2);


    // field-line   = field-name ":" OWS field-value OWS
    while (true) {
        size_t line_end = buf.find("\r\n");
        if (line_end == std::string_view::npos) {
            return std::unexpected(
                HttpParseError{
                    .status = HttpStatus::BadRequest,
                    .message = "Bad Request"
                }
            );
        }

        std::string_view line = buf.substr(0, line_end);
        buf.remove_prefix(line_end + 2);

        if (line.empty())
            break;


        if (view.headers_count >= MAX_REQUEST_HEADERS) {
            return std::unexpected(
                HttpParseError{
                    .status = HttpStatus::BadRequest,
                    .message = "Too many Headers"
                }
            );
        }

        size_t colon_pos = line.find_first_of(':');
        if (colon_pos == std::string_view::npos) {
            return std::unexpected(
                HttpParseError{
                    .status = HttpStatus::BadRequest,
                    .message = "Bad Request"
                }
            );
        }

        std::string_view name = line.substr(0, colon_pos);
        std::string_view value = line.substr(colon_pos + 1);

        if (name.empty() || (name.back() == ' ' || name.back() == '\t')) {
            return std::unexpected(
                HttpParseError{
                    .status = HttpStatus::BadRequest,
                    .message = "Bad Request"
                }
            );
        }

        while (!value.empty() && (value.front() == '\t' || value.front() == ' '))
            value.remove_prefix(1);

        while (!value.empty() && (value.back() == '\t' || value.back() == ' '))
            value.remove_suffix(1);

        view.headers[view.headers_count++] = HttpHeaderView{
            .name = name,
            .value = value
        };
    }

    return view;
}
