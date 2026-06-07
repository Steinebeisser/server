#include <cctype>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define PGS_ARGS \
    PGS_ARG(PGS_ARG_VALUE,    src_dir,         's', "src-dir",         "Source dir of the .ppp files", NULL) \
    PGS_ARG(PGS_ARG_VALUE,    out_file,        'o', "out-file",        "Output hpp file to write generated content to", NULL) \
    PGS_ARG(PGS_ARG_VALUE,    css,             'c', "css",             "The CSS file to use; supports [$ASSET:filename$] substitution", NULL) \
    PGS_ARG(PGS_ARG_VALUE,    assets_dir,      'a', "assets-dir",      "Directory of static assets to embed (no css here)", NULL) \
    PGS_ARG(PGS_ARG_VALUE,    assets_out,      'A', "assets-out",      "Output hpp file for generated assets", NULL) \
    PGS_ARG(PGS_ARG_VALUE,    skeleton,        'S', "skeleton",        "File to use for Skeleton, If not given looks for skeleton.ppp inside the src dir", NULL) \
    PGS_ARG(PGS_ARG_FLAG,     warn_on_missing, 'm', "warn-on-missing", "Only warns if keys from template are not present in the children and doesnt error", NULL) \
    PGS_ARG(PGS_ARG_OPTIONAL, help,            'h', "help",            "Show help message", NULL)

#define PGS_ARGS_IMPLEMENTATION
#include "third_party/pgs_args.h"


// ----------------------------------------------------------------------------
// utilities
// ----------------------------------------------------------------------------

static std::string read_file(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("cannot open: " + p.string());
    return { std::istreambuf_iterator<char>(f), {} };
}

static std::vector<unsigned char> read_file_bytes(const std::filesystem::path &p) {
    std::ifstream f(p, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("cannot open: " + p.string());
    return { std::istreambuf_iterator<char>(f), {} };
}

static std::string fnv1a(const std::string &s) {
    uint32_t h = 2166136261u;
    for (char ch : s) {
        unsigned char c = static_cast<unsigned char>(ch);
        h ^= c;
        h *= 16777619u;
    }
    char buf[9];
    std::snprintf(buf, sizeof buf, "%08x", h);
    return buf;
}

static std::string fnv1a_bytes(const std::vector<unsigned char> &b) {
    uint32_t h = 2166136261u;
    for (unsigned char c : b) {
        h ^= c;
        h *= 16777619u;
    }
    char buf[9];
    std::snprintf(buf, sizeof buf, "%08x", h);
    return buf;
}

static std::string trim(std::string s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::string to_upper(std::string s) {
    for (auto &c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static std::string to_ident(const std::string &filename) {
    std::string id;
    id.reserve(filename.size());
    for (char c : filename)
        id += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    return id;
}

static const char *mime_for_ext(const std::string &ext) {
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png")                   return "image/png";
    if (ext == ".gif")                   return "image/gif";
    if (ext == ".webp")                  return "image/webp";
    if (ext == ".svg")                   return "image/svg+xml";
    if (ext == ".ico")                   return "image/x-icon";
    if (ext == ".woff")                  return "font/woff";
    if (ext == ".woff2")                 return "font/woff2";
    if (ext == ".ttf")                   return "font/ttf";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".mp3")                   return "audio/mpeg";
    if (ext == ".ogg")                   return "audio/ogg";
    if (ext == ".wav")                   return "audio/wav";
    if (ext == ".flac")                  return "audio/flac";
    return "application/octet-stream";
}


// ----------------------------------------------------------------------------
// asset scanning
// ----------------------------------------------------------------------------

/*
 * Represents one file from the assets directory.
 * hashed_path is what gets embedded and served, e.g.:
 *   /assets/Riven_ValiantSwordSkin.a3f1bc92.jpg
 *
 * In your CSS, reference assets with:
 *   url("[$ASSET:Riven_ValiantSwordSkin.jpg$]")
 * which the generator replaces with the hashed path before hashing the CSS.
 */
struct AssetEntry {
    std::string              original_name; // Riven_ValiantSwordSkin.jpg
    std::string              ident;         // Riven_ValiantSwordSkin_jpg
    std::string              hashed_path;   // /assets/Riven_ValiantSwordSkin.a3f1bc92.jpg
    std::string              mime;
    std::string              hash;
    std::vector<unsigned char> data;
};

// Returns all assets found in the directory, sorted by name for determinism.
static std::vector<AssetEntry> scan_assets(const std::filesystem::path &assets_dir) {
    std::vector<AssetEntry> entries;

    for (const auto &entry : std::filesystem::directory_iterator(assets_dir)) {
        if (!entry.is_regular_file()) continue;

        const auto &p   = entry.path();
        std::string ext = p.extension().string();

        auto bytes       = read_file_bytes(p);
        std::string hash = fnv1a_bytes(bytes);
        std::string stem = p.stem().string();
        std::string name = p.filename().string();

        AssetEntry ae;
        ae.original_name = name;
        ae.ident         = to_ident(name);
        ae.hashed_path   = "/assets/" + stem + "." + hash + ext;
        ae.mime          = mime_for_ext(ext);
        ae.hash          = hash;
        ae.data          = std::move(bytes);

        std::cout << "asset: " << name << "  ->  " << ae.hashed_path << '\n';
        entries.push_back(std::move(ae));
    }

    std::sort(entries.begin(), entries.end(),
        [](const AssetEntry &a, const AssetEntry &b) {
            return a.original_name < b.original_name;
        });

    return entries;
}

// Build a vars map for [$ASSET:filename$] substitution in CSS / pages.
// Key format:  "ASSET:Riven_ValiantSwordSkin.jpg"
// Value:       "/assets/Riven_ValiantSwordSkin.a3f1bc92.jpg"
static std::map<std::string, std::string>
asset_vars(const std::vector<AssetEntry> &assets) {
    std::map<std::string, std::string> m;
    for (const auto &a : assets)
        m["ASSET:" + to_upper(a.original_name)] = a.hashed_path;
    return m;
}


// ----------------------------------------------------------------------------
// page parsing
// ----------------------------------------------------------------------------

/*
 * PAGE FORMAT:
 *
 * ---
 * |route: /
 * |pagetitle: Stein
 * |any-key: any value
 * ---
 * <h1>Body goes here</h1>
 *
 * The body goes into [$PAGE_CONTENT$] from the skeleton.
 * Lines in the frontmatter may begin with `|` for visual alignment;
 * the `|` is stripped before the key/value is parsed.
 * Keys are case-insensitive and are normalised to UPPERCASE internally.
 */

struct ParsedPage {
    std::filesystem::path               path;
    std::map<std::string, std::string>  vars;
    std::string                         route;
    std::string                         fragment_route;
    std::string                         body;
    std::string                         html;
    std::string                         fragment;
    std::size_t                         fragment_offset {0};
    std::size_t                         fragment_length {0};
};

static ParsedPage parse_page(const std::filesystem::path &path) {
    ParsedPage page;
    page.path = path;

    const std::string content = read_file(path);
    std::istringstream iss(content);

    enum class State { Start, InFrontmatter, InBody };
    State state = State::Start;

    std::string line;
    std::string body;

    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (state == State::Start) {
            std::string t = trim(line);
            if (t == "---") {
                state = State::InFrontmatter;
            } else if (!t.empty()) {
                state = State::InBody;
                body += line;
                body += '\n';
            }
            continue;
        }

        if (state == State::InFrontmatter) {
            std::string t = trim(line);
            if (t == "---") {
                state = State::InBody;
                continue;
            }
            if (t.empty()) continue;

            if (t.front() == '|') t.erase(0, 1);

            size_t colon = t.find(':');
            if (colon == std::string::npos) continue;

            std::string key = trim(t.substr(0, colon));
            std::string val = trim(t.substr(colon + 1));
            if (!key.empty())
                page.vars[to_upper(std::move(key))] = std::move(val);
            continue;
        }

        body += line;
        body += '\n';
    }

    if (!body.empty() && body.back() == '\n') body.pop_back();
    page.body = std::move(body);

    auto it = page.vars.find("ROUTE");
    if (it == page.vars.end() || it->second.empty())
        throw std::runtime_error("page is missing required `route`: " + path.string());
    page.route = it->second;
    if (page.route.back() != '/')
        page.route.append(1, '/');
    std::cout << page.route << '\n';
    page.fragment_route = "/fragment" + page.route;

    return page;
}


// ----------------------------------------------------------------------------
// template substitution
// ----------------------------------------------------------------------------

/*
 * TEMPLATE FORMAT:
 *
 * [$KEY$]           - replaces with vars["KEY"] - error if missing unless --warn-on-missing
 * [$KEY(default)$]  - replaces with vars["KEY"] - uses "default" if missing
 *
 * Also used for CSS asset substitution:
 * [$ASSET:filename.jpg$]  - replaced with /assets/filename.HASH.jpg
 */

static std::string apply_template(
    const std::string &templ,
    const std::map<std::string, std::string> &vars,
    bool warn_on_missing,
    const std::filesystem::path &source
) {
    std::string out;
    out.reserve(templ.size() * 2);

    size_t i = 0;
    while (i < templ.size()) {
        size_t open = templ.find("[$", i);
        if (open == std::string::npos) {
            out.append(templ, i, std::string::npos);
            break;
        }
        size_t close = templ.find("$]", open + 2);
        if (close == std::string::npos) {
            out.append(templ, i, std::string::npos);
            break;
        }

        out.append(templ, i, open - i);

        const std::string inner = templ.substr(open + 2, close - open - 2);

        std::string key;
        std::string default_value;
        bool        has_default = false;

        size_t paren = inner.find('(');
        if (paren != std::string::npos && !inner.empty() && inner.back() == ')') {
            key           = inner.substr(0, paren);
            default_value = inner.substr(paren + 1, inner.size() - paren - 2);
            has_default   = true;
        } else {
            key = inner;
        }

        key = to_upper(trim(std::move(key)));

        auto it = vars.find(key);
        if (it != vars.end()) {
            out += it->second;
        } else if (has_default) {
            out += default_value;
        } else {
            std::string msg = "missing key '" + key + "' in " + source.string();
            if (warn_on_missing) {
                std::cerr << "warning: " << msg << " (substituting empty string)\n";
            } else {
                throw std::runtime_error(msg);
            }
        }

        i = close + 2;
    }

    return out;
}


// ----------------------------------------------------------------------------
// raw-string delimiter
// ----------------------------------------------------------------------------

static std::string pick_raw_delimiter(const std::string &payload) {
    for (int n = 0; n < 10000; ++n) {
        std::string delim    = "PPP" + std::to_string(n);
        std::string sentinel = ")" + delim + "\"";
        if (payload.find(sentinel) == std::string::npos) return delim;
    }
    throw std::runtime_error("could not pick a unique raw-string delimiter");
}


// ----------------------------------------------------------------------------
// assets header codegen
// ----------------------------------------------------------------------------

static std::string generate_assets_header(const std::vector<AssetEntry> &assets) {
    std::string out;
    out += "#pragma once\n";
    out += "// Auto-generated by ppp generator. Do not edit by hand.\n\n";
    out += "#include <cstddef>\n";
    out += "#include <string_view>\n\n";
    out += "namespace generated_assets {\n\n";
    out += "struct Asset {\n";
    out += "    const char          *path;\n";
    out += "    const char          *mime_type;\n";
    out += "    const unsigned char *data;\n";
    out += "    std::size_t          size;\n";
    out += "};\n\n";

    // one byte array per asset
    for (const auto &e : assets) {
        out += "inline constexpr unsigned char " + e.ident + "_data[] = {\n    ";
        for (std::size_t i = 0; i < e.data.size(); ++i) {
            char buf[6];
            std::snprintf(buf, sizeof buf, "0x%02x", e.data[i]);
            out += buf;
            if (i + 1 < e.data.size()) {
                out += ',';
                if ((i + 1) % 16 == 0) out += "\n    ";
                else                    out += ' ';
            }
        }
        out += "\n};\n\n";
    }

    // ASSET_PATH_<IDENT> macros so the router can build constexpr string_views
    for (const auto &e : assets)
        out += "#define ASSET_PATH_" + to_upper(e.ident)
             + " \"" + e.hashed_path + "\"\n";
    out += '\n';

    for (const auto &e : assets) {
        out += "inline constexpr std::string_view " + e.ident
            + "_path { ASSET_PATH_" + to_upper(e.ident) + " };\n";
    }
    out += '\n';

    out += "#define ASSET_ROUTES(H)";
    for (std::size_t i = 0; i < assets.size(); ++i) {
        out += " \\\n   { HttpMethod::Get, generated_assets::"
            + assets[i].ident + "_path, H }";
        if (i + 1 < assets.size()) out += ',';
    }

    out += "\n\n";

    out += "inline constexpr Asset assets[] = {\n";
    for (const auto &e : assets) {
        out += "    { \"" + e.hashed_path + "\", ";
        out += "\"" + e.mime + "\", ";
        out += e.ident + "_data, ";
        out += "sizeof(" + e.ident + "_data) },\n";
    }
    out += "};\n\n";
    out += "inline constexpr std::size_t asset_count = "
           "sizeof(assets) / sizeof(assets[0]);\n\n";
    out += "} // namespace generated_assets\n";

    return out;
}


// ----------------------------------------------------------------------------
// pages header codegen
// ----------------------------------------------------------------------------

static std::string generate_pages_header(
    const std::filesystem::path     &pages_dir,
    const std::filesystem::path     &skeleton,
    const char                      *css_path,
    const std::vector<AssetEntry>   &assets,
    bool                             warn_on_missing
) {
    const std::string skel_raw = read_file(skeleton);

    // Build the asset substitution vars (ASSET:FILENAME -> hashed path).
    // These are available both when processing the CSS and when rendering pages.
    auto avars = asset_vars(assets);

    // Process CSS: substitute [$ASSET:...‌$] references, then hash the result.
    std::string css_hash;
    std::string css_processed;
    if (css_path) {
        std::string css_raw = read_file(css_path);
        // CSS gets asset vars only (no page vars at this stage)
        css_processed = apply_template(css_raw, avars, warn_on_missing,
                                       std::filesystem::path(css_path));
        css_hash = fnv1a(css_processed);
        std::cout << "css hash: " << css_hash << '\n';
    }

    std::vector<ParsedPage> pages;

    for (const auto &entry : std::filesystem::directory_iterator(pages_dir)) {
        if (!entry.is_regular_file())                            continue;
        if (entry.path().extension() != ".ppp")                 continue;
        if (std::filesystem::equivalent(entry.path(), skeleton)) continue;
        pages.push_back(parse_page(entry.path()));
    }
    for (auto &page : pages) {
        auto tvars = page.vars;
        for (const auto &[k, v] : avars) {
            tvars.emplace(k, v);
        }
        if (!css_hash.empty()) {
            tvars["CSS_HASH"] = css_hash;
        }

        // Generate a localized temporary layout to get a deterministic unique hash
        std::string initial_frag = apply_template(page.body, tvars, true, page.path);
        std::string fhash = fnv1a(initial_frag);
        page.fragment_route = "/fragment/" + fhash + page.route;
    }

    // Pass 2: Now that every single page's `fragment_route` is definitively known globally, 
    // we process the page bodies a second time. This allows $FRAGMENT_ROUTE variations inside page files!
    for (auto &page : pages) {
        auto tvars = page.vars;

        for (const auto &[k, v] : avars)
            tvars.emplace(k, v);

        // Build global routing map entries for all fragments
        for (const auto &other : pages) {
            tvars["ACTIVE:" + to_upper(other.route)] = (other.route == page.route) ? "active" : "";
            tvars["FRAGMENT_ROUTE:" + to_upper(other.route)] = other.fragment_route;
        }

        if (!css_hash.empty())
            tvars["CSS_HASH"] = css_hash;

        // Render the actual fragment containing fully populated paths
        page.fragment = apply_template(page.body, tvars, warn_on_missing, page.path);

        std::cout << "rendered: " << page.path.filename().string()
                  << "  ->  " << page.route
                  << "  (fragment: " << page.fragment_route << ")\n";
    }

    // Pass 3: Construct full parent container layout files (Skeleton)
    for (auto &page : pages) {
        auto tvars = page.vars;

        for (const auto &[k, v] : avars)
            tvars.emplace(k, v);

        for (const auto &other : pages) {
            tvars["ACTIVE:" + to_upper(other.route)] = (other.route == page.route) ? "active" : "";
            tvars["FRAGMENT_ROUTE:" + to_upper(other.route)] = other.fragment_route;
        }

        if (!css_hash.empty())
            tvars["CSS_HASH"] = css_hash;

        tvars["PAGE_CONTENT"] = page.fragment;

        page.html = apply_template(skel_raw, tvars, warn_on_missing, page.path);

        auto pos = page.html.find(page.fragment);
        if (pos == std::string::npos)
            throw std::runtime_error("fragment not found in full HTML: " + page.path.string());

        page.fragment_offset = pos;
        page.fragment_length = page.fragment.size();
    }

    std::string out;
    out += "#pragma once\n";
    out += "// Auto-generated by ppp generator. Do not edit by hand.\n\n";
    out += "#include <cstddef>\n";
    out += "#include <string_view>\n\n";
    out += "namespace generated_pages {\n\n";
    out += "struct Page {\n";
    out += "    const char  *route;\n";
    out += "    const char  *fragment_route;\n";
    out += "    const char  *html;\n";
    out += "    std::size_t  fragment_offset;\n";
    out += "    std::size_t  fragment_length;\n";
    out += "\n";
    out += "    constexpr std::string_view fragment() const noexcept {\n";
    out += "        return { html + fragment_offset, fragment_length };\n";
    out += "    }\n";
    out += "};\n\n";

    if (!css_hash.empty()) {
        out += "inline constexpr const char *css_hash = \"" + css_hash + "\";\n\n";
        out += "#define CSS_HASH_MACRO \"" + css_hash + "\"\n\n";

        const std::string css_delim = pick_raw_delimiter(css_processed);
        out += "inline constexpr std::string_view css_content { R\""
             + css_delim + "(" + css_processed + ")" + css_delim + "\" };\n\n";
    }

    out += "#define PAGE_ROUTES(H)";
    for (std::size_t i = 0; i < pages.size(); ++i) {
        out += " \\\n    { HttpMethod::Get, \"" + pages[i].route + "\", H },";
        out += " \\\n    { HttpMethod::Get, \"" + pages[i].fragment_route + "\", H }";
        if (i + 1 < pages.size()) out += ',';
    }
    out += "\n\n";

    out += "inline constexpr Page pages[] = {\n";
    for (const auto &p : pages) {
        const std::string delim = pick_raw_delimiter(p.html);
        out += "    { \"" + p.route + "\", ";
        out += "\"" + p.fragment_route + "\", ";
        out += "R\"" + delim + "(" + p.html + ")" + delim + "\", ";
        out += std::to_string(p.fragment_offset) + ", ";
        out += std::to_string(p.fragment_length) + " },\n";
    }
    out += "};\n\n";
    out += "inline constexpr std::size_t page_count = "
           "sizeof(pages) / sizeof(pages[0]);\n\n";
    out += "} // namespace generated_pages\n";

    return out;
}


// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------

int main(int argc, char **argv) {
    pgs_args_t args {};
    if (!pgs_args_parse(&args, argc, argv, false)) {
        std::cerr << "Failed to parse args\n";
        return 1;
    }

    if (args.help_present) {
        if (args.help_value) pgs_args_print_help_specific_name(args.help_value);
        else                 pgs_args_print_help();
        return 0;
    }

    if (!args.src_dir_present || !args.out_file_present) {
        std::cerr << "Usage: " << argv[0]
                  << " -s [src-dir] -o [out-file] {-c [css]} {-a [assets-dir] -A [assets-out]} {-S [skeleton]} {-m}\n";
        return 1;
    }

    std::filesystem::path pages_dir { args.src_dir_value };
    if (!std::filesystem::exists(pages_dir) || !std::filesystem::is_directory(pages_dir)) {
        std::cerr << "src directory doesnt exist or is not a directory\n";
        return 1;
    }

    std::filesystem::path skeleton = args.skeleton_present
        ? pages_dir / args.skeleton_value
        : pages_dir / "skeleton.ppp";

    if (!std::filesystem::exists(skeleton) || !std::filesystem::is_regular_file(skeleton)) {
        std::cerr << "Skeleton file couldnt be found at: " << skeleton
                  << "\n    Make sure it exists and is a file\n";
        return 1;
    }

    if (args.assets_dir_present != args.assets_out_present) {
        std::cerr << "error: -a and -A must be provided together\n";
        return 1;
    }

    const char *css_path = args.css_present ? args.css_value : nullptr;
    if (css_path)
        std::cout << "css file at: " << css_path << '\n';

    try {
        std::vector<AssetEntry> assets;
        if (args.assets_dir_present) {
            std::filesystem::path adir { args.assets_dir_value };
            if (!std::filesystem::exists(adir) || !std::filesystem::is_directory(adir)) {
                std::cerr << "assets directory doesnt exist: " << adir << '\n';
                return 1;
            }
            assets = scan_assets(adir);
        }

        if (args.assets_out_present) {
            const std::string ahdr = generate_assets_header(assets);
            std::ofstream af(args.assets_out_value, std::ios::binary);
            if (!af.is_open()) {
                std::cerr << "Cannot open assets output file: " << args.assets_out_value << '\n';
                return 1;
            }
            af << ahdr;
            if (!af.good()) {
                std::cerr << "Error writing assets file: " << args.assets_out_value << '\n';
                return 1;
            }
            std::cout << "wrote " << args.assets_out_value << '\n';
        }

        const std::string pages_hdr = generate_pages_header(
            pages_dir, skeleton, css_path, assets,
            args.warn_on_missing_present
        );

        std::ofstream of(args.out_file_value, std::ios::binary);
        if (!of.is_open()) {
            std::cerr << "Cannot open output file for writing: " << args.out_file_value << '\n';
            return 1;
        }
        of << pages_hdr;
        if (!of.good()) {
            std::cerr << "Error while writing output file: " << args.out_file_value << '\n';
            return 1;
        }
        std::cout << "wrote " << args.out_file_value << '\n';

    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
