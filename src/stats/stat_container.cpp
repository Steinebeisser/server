#ifdef ENABLE_STATS
#include "stat_container.hpp"
#include "server/request.hpp"
#include "stats/stats.hpp"
#include <cstring>
#include <ctime>
#include <sqlite3.h>

bool StatContainer::init(const char *db_path) {
    sqlite3_open(db_path, &db_);
    if (!db_) {
        PGS_LOG_FATAL("Failed to create sqlite db");
        return false;
    }

    // https://sqlite.org/forum/info/f832398c19d30a4a
    sqlite3_exec(db_, "PRAGMA journal_mode = OFF",  nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA synchronous = 0",     nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA cache_size = -64000", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA temp_store = MEMORY", nullptr, nullptr, nullptr);

    sqlite3_stmt* create_table_stmt = NULL;
    const char *test = NULL;
    sqlite3_prepare_v2(db_, CREATE_STMT_RAW, strlen(CREATE_STMT_RAW) + 1, &create_table_stmt, &test);

    if (!create_table_stmt) {
        PGS_LOG_ERROR("Failed to create table stmt, not tracking stats");
        return false;
    }

    int res = sqlite3_step(create_table_stmt);
    sqlite3_finalize(create_table_stmt);
    if (res != SQLITE_DONE) {
        PGS_LOG_ERROR("Failed to execute table stmt, not tracking stats, ERR CODE: %d", res);
        return false;
    }


    sqlite3_prepare_v2(db_, INSERT_STMT_RAW, strlen(INSERT_STMT_RAW) + 1, &insert_statement_, &test);

    if (!insert_statement_) {
        PGS_LOG_ERROR("Failed to create table stmt, not tracking stats");
        return false;
    }

    return true;
}

StatRecord* StatContainer::get() {
    return &buffer_[write_idx_++ % RING_SIZE];
}

void StatContainer::maybe_flush() {
    int pending = write_idx_ - flush_idx_;
    if (pending == 0) return;

    if (pending >= BATCH_CAP) {
        flush();
        return;
    }
    timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC, &cur_time);
    if (cur_time.tv_sec - last_flush_ >= FLUSH_SECONDS) {
        flush();
    }
}

void StatContainer::flush() {
    sqlite3_exec(db_, "BEGIN", nullptr, nullptr, nullptr);

    while (flush_idx_ < write_idx_) {
        StatRecord& record = buffer_[flush_idx_ % RING_SIZE];
        int res = record.reset_and_bind(*insert_statement_);
        if (res != SQLITE_OK) {
            PGS_LOG_ERROR("Failed to bind stat record: %d", res);
            break;
        }
        res = sqlite3_step(insert_statement_);
        if (res != SQLITE_DONE) {
            PGS_LOG_ERROR("Failed to insert stat record: %d", res);
            break;
        }
        flush_idx_++;
    }

    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);

    timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC, &cur_time);
    last_flush_ = cur_time.tv_sec;
}

#endif // ENABLE_STATS
