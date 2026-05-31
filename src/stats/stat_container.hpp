#pragma once
#ifdef ENABLE_STATS

#include <optional>
#include <sqlite3.h>
#include "stats/stats.hpp"
#include "third_party/pgs_macros.h"

class StatContainer {
public:
    static constexpr int BATCH_CAP = 10'000;
    static constexpr int RING_SIZE = PGS_KIB(16);
    static constexpr int FLUSH_SECONDS = 300;

    StatContainer() = default;
    bool init(const char *db_path);

    StatRecord* get();
    void maybe_flush();
private:
    void flush();

    int write_idx_ {0};
    int flush_idx_ {0};

    time_t last_flush_ {0};

    StatRecord buffer_[RING_SIZE] {};

    sqlite3      *db_        {nullptr};
    sqlite3_stmt *insert_statement_ {nullptr};
};
#endif
