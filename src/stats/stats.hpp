#pragma once
#ifdef ENABLE_STATS

#include "http/types.hpp"
#include <cstddef>
#include <cstdint>
#include <sqlite3.h>

// https://pypi.org/project/iso3166-2/

typedef char endpoint_string[64];
typedef char country_code_string[3];
typedef char subdivision_code_string[6];

#define NORMAL_STAT_FIELDS \
    X(timestamp,      int64_t,          INTEGER)     \
    X(endpoint,       endpoint_string,  VARCHAR(64)) \
    X(method,         HttpMethod,       INTEGER)     \
    X(latency_ns,     int64_t,          INTEGER)     \
    X(request_bytes,  int64_t,          INTEGER)     \
    X(response_bytes, int64_t,          INTEGER)     \
    X(status_code,    HttpStatus,       INTEGER)     \
    X(version,        HttpVersion,      INTEGER)

#ifdef ENABLE_GEOIP
#define GEOIP_STAT_FIELDS \
    X(country_code,     country_code_string, VARCHAR(3)) \
    X(subdivision_code, subdivision_code_string, VARCHAR(6))
#else
#define GEOIP_STAT_FIELDS
#endif

#define STAT_FIELDS \
        NORMAL_STAT_FIELDS \
        GEOIP_STAT_FIELDS

inline int bind_value(sqlite3_stmt &stmt, int idx, int64_t value) {
    return sqlite3_bind_int64(&stmt, idx, value);
}


template <size_t N>
inline int bind_value(sqlite3_stmt &stmt, int idx, const char (&value)[N]) {
    return sqlite3_bind_text(&stmt, idx, value, -1, SQLITE_STATIC);
}

template <typename T>
concept IsEnum = std::is_enum_v<T>;

template<IsEnum T>
int bind_value(sqlite3_stmt &stmt, int idx, T value) {
    return sqlite3_bind_int64(&stmt, idx, static_cast<int64_t>(value));
}


struct StatRecord {
#define X(name, cpp_type, sqlite_type) \
    cpp_type name;
    STAT_FIELDS
#undef X

    int reset_and_bind(sqlite3_stmt &stmt) {
        int res = sqlite3_reset(&stmt);
        if (res != SQLITE_OK)
            return res;

        int idx = 1;
#define X(name, cpp_type, sqlite_type) \
        res = bind_value(stmt, idx++, name); \
        if (res != SQLITE_OK) \
            return res;
    STAT_FIELDS
#undef X
        return res;
    }
};

#define STAT_TABLE_NAME "stats"

static constexpr const char* CREATE_STMT_RAW =
    "CREATE TABLE IF NOT EXISTS " STAT_TABLE_NAME " ("
    "id INTEGER,"
#define X(name, cpp_type, sqlite_type) \
    #name " " #sqlite_type ","
    STAT_FIELDS
#undef  X
    " PRIMARY KEY (id) );"
    ;

static constexpr const char* INSERT_STMT_RAW =
    "INSERT INTO " STAT_TABLE_NAME
    " ("
#define X(name, cpp_type, sqlite_type) \
    #name ","
    STAT_FIELDS
#undef X
    "id)"
    "VALUES ("
#define X(name, cpp_type, sqlite_type) \
        "?,"
    STAT_FIELDS
#undef X
    "NULL);";


struct Stats {
    size_t request_amount;
#ifdef ENABLE_GEOIP
#endif
};

#endif // ENABLE_STATS
