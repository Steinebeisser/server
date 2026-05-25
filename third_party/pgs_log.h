/* PGS_LOG -v0.4.5 - Public Domain - https://github.com/Steinebeisser/pgs/blob/master/pgs_log_h.h

    simple/fast logging library

    Most things you will use:

        Logging Macros:
            PGS_LOG_DEBUG(fmt, ...)
            PGS_LOG_INFO(fmt, ...)
            PGS_LOG_WARN(fmt, ...)
            PGS_LOG_ERROR(fmt, ...)
            PGS_LOG_FATAL(fmt, ...)

        Set minimum log level:
            pgs_log_minimal_log_level

        Toggle logging on/off at runtime:
            pgs_log_toggle(bool enabled)

        Check last error (v0.2.0+):
            Pgs_Log_Error_Detail err = pgs_log_get_last_error();
            pgs_log_print_error_detail();

    Placeholder formatting (for custom PGS_LOG_FORMAT):
        %L = LOG LEVEL
        %T = TIMESTAMP
        %F = FILE
        %l = LINE
        %M = MESSAGE
*/
#pragma GCC system_header

#ifndef PGS_LOG_H
#define PGS_LOG_H

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#   include <direct.h>
#endif


/*
 * TODO:    - log colors
 *          - rotating log files (time & size)
 *          - thread safety
 *          - stuff like NOB_DEPRECATED warning
 *          - embedded mode also specialized for different micro controllers (less sizes, and less includes, no printf, no file etc)
*/

#define PGS_LOG_LEVEL_DEBUG 0
#define PGS_LOG_LEVEL_INFO  1
#define PGS_LOG_LEVEL_WARN  2
#define PGS_LOG_LEVEL_ERROR 3
#define PGS_LOG_LEVEL_FATAL 4

#if defined(__GNUC__) || defined(__clang__)

#   define PGS_LOG_PRINTF_LIKE(fmt_index, first_arg)  __attribute__((format(printf, fmt_index, first_arg)))

#elif defined(_MSC_VER)

#   define PGS_LOG_PRINTF_LIKE(fmt_index, first_arg)

#else

#   define PGS_LOG_PRINTF_LIKE(fmt_index, first_arg)

#endif


#ifndef PGS_LOG_TIMESTAMP_FORMAT
#   define PGS_LOG_TIMESTAMP_FORMAT "%d-%m-%Y %H:%M:%S"
#endif
#ifndef PGS_LOG_FORMAT
#   define PGS_LOG_FORMAT "[%G] [%L] %T %F:%l - \"%M\""
#endif
#ifndef PGS_LOG_MAX_ENTRY_LEN
#   define PGS_LOG_MAX_ENTRY_LEN 2048
#endif
#ifndef PGS_LOG_MAX_PATH_LEN
#   define PGS_LOG_MAX_PATH_LEN 512
#endif
#ifndef PGS_LOG_MAX_TIMESTAMP_LEN
#   define PGS_LOG_MAX_TIMESTAMP_LEN 64
#endif
#ifndef PGS_LOG_TEMP_BUFFERS
#   define PGS_LOG_TEMP_BUFFERS 2
#endif
#ifndef PGS_LOG_MAX_TEMP_BUFFER_LEN
#   define PGS_LOG_MAX_TEMP_BUFFER_LEN 1024
#endif
#ifndef PGS_LOG_PATH
#   define PGS_LOG_PATH "logs/%d-%m-%Y.log"
#endif
#ifndef PGS_LOG_APPEND
#   define PGS_LOG_APPEND true
#endif
#ifndef PGS_LOG_OVERRIDE
#   define PGS_LOG_OVERRIDE false
#endif
#ifndef PGS_LOG_ENABLED_FILE
#   define PGS_LOG_ENABLED_FILE true
#endif
#ifndef PGS_LOG_ENABLED_STDOUT
#   define PGS_LOG_ENABLED_STDOUT true
#endif
#ifndef PGS_LOG_MAX_FD
#   define PGS_LOG_MAX_FD 8
#endif
#ifndef PGS_LOG_USE_DETAIL_ERROR
#   define PGS_LOG_USE_DETAIL_ERROR false
#endif
#ifndef PGS_LOG_ERROR_MESSAGE_SIZE
#   define PGS_LOG_ERROR_MESSAGE_SIZE 1024
#endif
#ifndef PGS_LOG_ENABLED
#   define PGS_LOG_ENABLED true
#endif
#ifndef PGS_LOG_MAX_FILENAME_NUMBER
#   define PGS_LOG_MAX_FILENAME_NUMBER 99
#endif
#ifndef PGS_LOG_MAX_OUTPUT_BUFFER_SIZE
#   define PGS_LOG_MAX_OUTPUT_BUFFER_SIZE 65536
#endif
#ifndef PGS_LOG_ENABLED_BUFFERING
#   define PGS_LOG_ENABLED_BUFFERING true
#endif
#ifndef PGS_LOG_BUFFER_INSTA_WRITE_IF_TOO_LARGE
#   define PGS_LOG_BUFFER_INSTA_WRITE_IF_TOO_LARGE false
#endif
#ifndef PGS_LOG_BUFFER_INSTA_WRITE_TERMINAL
#   define PGS_LOG_BUFFER_INSTA_WRITE_TERMINAL true
#endif
#ifndef PGS_LOG_COMPILE_LEVEL
#   define PGS_LOG_COMPILE_LEVEL PGS_LOG_LEVEL_DEBUG
#endif

typedef enum {
    PGS_LOG_DEBUG = PGS_LOG_LEVEL_DEBUG,
    PGS_LOG_INFO  = PGS_LOG_LEVEL_INFO,
    PGS_LOG_WARN  = PGS_LOG_LEVEL_WARN,
    PGS_LOG_ERROR = PGS_LOG_LEVEL_ERROR,
    PGS_LOG_FATAL = PGS_LOG_LEVEL_FATAL,
} Pgs_Log_Level;

typedef enum {
    PGS_LOG_OK = 0,
    PGS_LOG_ERR,
    PGS_LOG_ERR_FILE,
    PGS_LOG_ERR_IO,
} Pgs_Log_Error;

#if PGS_LOG_USE_DETAIL_ERROR
typedef struct {
    Pgs_Log_Error type;
    char message[PGS_LOG_ERROR_MESSAGE_SIZE];
    int errno_value;
} Pgs_Log_Error_Detail;
#else
typedef struct {
} Pgs_Log_Error_Detail;
#endif

typedef struct {
    FILE *fd;
#if PGS_LOG_ENABLED_BUFFERING
    char buffer[PGS_LOG_MAX_OUTPUT_BUFFER_SIZE];
    size_t buf_pos;
#endif
} Pgs_Log_Output;

extern Pgs_Log_Level pgs_log_minimal_log_level;

Pgs_Log_Error_Detail pgs_log_get_last_error(void);
Pgs_Log_Error pgs_log_set_last_error(Pgs_Log_Error type, const char *message, int errn);

Pgs_Log_Error pgs_log(Pgs_Log_Level level, const char *tag, size_t tag_len, const char *file, size_t file_len, const char *line, size_t line_len, const char *fmt, ...) PGS_LOG_PRINTF_LIKE(8, 9);

const char *pgs_log_level_to_string(Pgs_Log_Level level);
const char *pgs_log_timestamp_string(void);

Pgs_Log_Error pgs_log_add_fd_output(FILE *file);
Pgs_Log_Error pgs_log_remove_fd_output(FILE *file);

Pgs_Log_Error pgs_log_mkdir_if_not_exists(const char *path);
Pgs_Log_Error pgs_log_create_dirs_for_path(const char *fullpath);

bool pgs_log_file_exists(const char *file_path);
int pgs_log_get_last_occurence_of(const char ch, const char *string);

void pgs_log_cleanup(void);

void pgs_log_toggle(bool enabled);

void pgs_log_print_error_detail(void);

char *pgs_log_temp_sprintf(const char *format, ...);

Pgs_Log_Error pgs_log_write_output(const char *str, size_t len);
Pgs_Log_Error pgs_log_flush(void);

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

#if PGS_LOG_ENABLED

#   if PGS_LOG_COMPILE_LEVEL <= PGS_LOG_LEVEL_DEBUG
#       define PGS_LOG_DEBUG(fmt, ...)                                                                                  \
            do {                                                                                                        \
                if (PGS_LOG_CURRENT_TU_ENABLED) {                                                                       \
                    pgs_log(PGS_LOG_DEBUG, PGS_LOG_CURRENT_TU_TAG, (size_t)(sizeof(PGS_LOG_CURRENT_TU_TAG) - 1),        \
                            __FILE__, (size_t)(sizeof(__FILE__) - 1),                                                   \
                    STRINGIFY(__LINE__), (size_t)(sizeof(STRINGIFY(__LINE__)) - 1), fmt, ##__VA_ARGS__);                \
                }                                                                                                       \
            } while(0)
#   else
#       define PGS_LOG_DEBUG(fmt, ...)
#   endif

#   if PGS_LOG_COMPILE_LEVEL <= PGS_LOG_LEVEL_INFO
#       define PGS_LOG_INFO(fmt, ...)                                                                                   \
            do {                                                                                                        \
                if (PGS_LOG_CURRENT_TU_ENABLED) {                                                                       \
                    pgs_log(PGS_LOG_INFO, PGS_LOG_CURRENT_TU_TAG, (size_t)(sizeof(PGS_LOG_CURRENT_TU_TAG) - 1),         \
                            __FILE__, (size_t)(sizeof(__FILE__) - 1),                                                   \
                    STRINGIFY(__LINE__), (size_t)(sizeof(STRINGIFY(__LINE__)) - 1), fmt, ##__VA_ARGS__);                \
                }                                                                                                       \
            } while(0)
#   else
#       define PGS_LOG_INFO(fmt, ...)
#   endif

#   if PGS_LOG_COMPILE_LEVEL <= PGS_LOG_LEVEL_WARN
#       define PGS_LOG_WARN(fmt, ...)                                                                                   \
            do {                                                                                                        \
                if (PGS_LOG_CURRENT_TU_ENABLED) {                                                                       \
                    pgs_log(PGS_LOG_WARN, PGS_LOG_CURRENT_TU_TAG, (size_t)(sizeof(PGS_LOG_CURRENT_TU_TAG) - 1),         \
                            __FILE__, (size_t)(sizeof(__FILE__) - 1),                                                   \
                    STRINGIFY(__LINE__), (size_t)(sizeof(STRINGIFY(__LINE__)) - 1), fmt, ##__VA_ARGS__);                \
                }                                                                                                       \
            } while(0)
#   else
#       define PGS_LOG_WARN(fmt, ...)
#   endif

#   if PGS_LOG_COMPILE_LEVEL <= PGS_LOG_LEVEL_ERROR
#       define PGS_LOG_ERROR(fmt, ...)                                                                                  \
            do {                                                                                                        \
                if (PGS_LOG_CURRENT_TU_ENABLED) {                                                                       \
                    pgs_log(PGS_LOG_ERROR, PGS_LOG_CURRENT_TU_TAG, (size_t)(sizeof(PGS_LOG_CURRENT_TU_TAG) - 1),        \
                            __FILE__, (size_t)(sizeof(__FILE__) - 1),                                                   \
                    STRINGIFY(__LINE__), (size_t)(sizeof(STRINGIFY(__LINE__)) - 1), fmt, ##__VA_ARGS__);                \
                }                                                                                                       \
            } while(0)
#   else
#       define PGS_LOG_ERROR(fmt, ...)
#   endif

#   if PGS_LOG_COMPILE_LEVEL <= PGS_LOG_LEVEL_FATAL
#       define PGS_LOG_FATAL(fmt, ...)                                                                                  \
            do {                                                                                                        \
                if (PGS_LOG_CURRENT_TU_ENABLED) {                                                                       \
                    pgs_log(PGS_LOG_FATAL, PGS_LOG_CURRENT_TU_TAG, (size_t)(sizeof(PGS_LOG_CURRENT_TU_TAG) - 1),        \
                            __FILE__, (size_t)(sizeof(__FILE__) - 1),                                                   \
                    STRINGIFY(__LINE__), (size_t)(sizeof(STRINGIFY(__LINE__)) - 1), fmt, ##__VA_ARGS__);                \
                }                                                                                                       \
            } while(0)
#   else
#       define PGS_LOG_FATAL(fmt, ...)
#   endif


#else //PGS_LOG_ENABLED
#define PGS_LOG_DEBUG(fmt, ...)
#define PGS_LOG_INFO(fmt, ...)
#define PGS_LOG_WARN(fmt, ...)
#define PGS_LOG_ERROR(fmt, ...)
#define PGS_LOG_FATAL(fmt, ...)
#endif


#endif // PGS_LOG_H

#undef PGS_LOG_CURRENT_TU_TAG
#ifdef PGS_LOG_TU_TAG
#   define PGS_LOG_CURRENT_TU_TAG PGS_LOG_TU_TAG
#else
#   define PGS_LOG_CURRENT_TU_TAG "GLOBAL"
#endif

#undef PGS_LOG_CURRENT_TU_ENABLED
#ifdef PGS_LOG_TU_ENABLED
#   define PGS_LOG_CURRENT_TU_ENABLED PGS_LOG_TU_ENABLED
#else
#   define PGS_LOG_CURRENT_TU_ENABLED true
#endif

#ifdef PGS_LOG_IMPLEMENTATION

static size_t pgs_max_tag_width = 6;

#if defined(_WIN32)
    #include <io.h>
    #define pgs_write _write
#else
    #include <unistd.h>
    #define pgs_write write
#endif

void sigint_handler(int signo) {
    pgs_log_cleanup();
    (void)signo;
    exit(130);
}

Pgs_Log_Level pgs_log_minimal_log_level = PGS_LOG_DEBUG;

static Pgs_Log_Output pgs_outputs[PGS_LOG_MAX_FD];
static int pgs_output_count = 0;
static bool pgs_log_initialized = false;
static bool pgs_log_is_enabled = PGS_LOG_ENABLED;

static char pgs_log_cached_timestamp[PGS_LOG_MAX_TIMESTAMP_LEN];
static time_t pgs_log_last_timestamp = 0;

#if PGS_LOG_USE_DETAIL_ERROR
static Pgs_Log_Error_Detail pgs_log_last_error = { .type = PGS_LOG_OK, .message = {0}, .errno_value = 0, };
#else
static Pgs_Log_Error_Detail pgs_log_last_error = { };
#endif


Pgs_Log_Error_Detail pgs_log_get_last_error(void) {
    return pgs_log_last_error;
}

Pgs_Log_Error pgs_log_set_last_error(Pgs_Log_Error type, const char *msg, int errn) {
#if PGS_LOG_USE_DETAIL_ERROR
    pgs_log_last_error.type = type;
    pgs_log_last_error.errno_value = errn;
    if (errn != 0) {
        snprintf(pgs_log_last_error.message, PGS_LOG_ERROR_MESSAGE_SIZE, "%s: %s", msg, strerror(errn));
    } else {
        strncpy(pgs_log_last_error.message, msg, PGS_LOG_ERROR_MESSAGE_SIZE - 1);
        pgs_log_last_error.message[PGS_LOG_ERROR_MESSAGE_SIZE - 1] = '\0';
    }
#endif
    (void)msg;
    (void)errn;
    return type;
}

Pgs_Log_Error pgs_log_init_if_needed() {
    if (!pgs_log_initialized) {
        signal(SIGINT, sigint_handler);
#if PGS_LOG_ENABLED_STDOUT
        if (pgs_log_add_fd_output(stdout) != PGS_LOG_OK) 
            return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "Failed to add STDOUT as output", 0);
#endif
#if PGS_LOG_ENABLED_FILE
        char filename[PGS_LOG_MAX_PATH_LEN];
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);

        if (strftime(filename, PGS_LOG_MAX_PATH_LEN, PGS_LOG_PATH, tm_info) == 0) {
            return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "Failed to format log filename", 0);
        }

        if (pgs_log_create_dirs_for_path(filename) != PGS_LOG_OK) {
            return pgs_log_set_last_error(PGS_LOG_ERR_FILE, pgs_log_temp_sprintf("Failed to create dirs for path %s", filename), 0);
        }

        FILE *log_file = NULL;

        if (pgs_log_file_exists(filename)) {
#if PGS_LOG_APPEND
            log_file = fopen(filename, "a");
#elif PGS_LOG_OVERRIDE
            log_file = fopen(filename, "w");
#else
            size_t base_size = PGS_LOG_MAX_PATH_LEN * 2 / 3;
            size_t ext_size = PGS_LOG_MAX_PATH_LEN / 3;
            char base[base_size];
            char ext[ext_size];
            int file_ending_pos = pgs_log_get_last_occurence_of('.', filename);
            if (file_ending_pos == -1) {
                strncpy(base, filename,  base_size - 1);
                base[base_size - 1] = '\0';
                ext[0] = '\0';
            } else {
                strncpy(base, filename, file_ending_pos);
                base[file_ending_pos] = '\0';
                strncpy(ext, filename + file_ending_pos, ext_size - 1);
                ext[ext_size - 1] = '\0';
            }

            int number = 0;

            while (pgs_log_file_exists(filename)) {
                number += 1;
                if (number >= PGS_LOG_MAX_FILENAME_NUMBER)
                    return pgs_log_set_last_error(PGS_LOG_ERR, "Too many log files with the same name exist, change `PGS_LOG_MAX_FILENAME_NUMBER` or fix ur config", errno);
                snprintf(filename, PGS_LOG_MAX_PATH_LEN, "%s(%d)%s", base, number, ext);
            }
            log_file = fopen(filename, "w");
#endif
        } else {
             log_file = fopen(filename, "w");
        }

        if (!log_file) {
            return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "Failed to open log file", errno);
        }

        if (pgs_log_add_fd_output(log_file) != PGS_LOG_OK) {
            fclose(log_file);
            return pgs_log_set_last_error(PGS_LOG_ERR, "Failed to add log file to file descriptors", 0);
        }
#endif
        pgs_log_initialized = true;
        atexit(pgs_log_cleanup);
    }

    return pgs_log_set_last_error(PGS_LOG_OK, "Initialized logging", 0);
}


#if PGS_LOG_ENABLED

Pgs_Log_Error pgs_log(Pgs_Log_Level level, const char *tag, size_t tag_len, const char *file, size_t file_len, const char *line, size_t line_len, const char *fmt, ...) {

    if (!pgs_log_is_enabled)
        return pgs_log_set_last_error(PGS_LOG_OK, "Logging disabled", 0);

    if (level < pgs_log_minimal_log_level)
        return pgs_log_set_last_error(PGS_LOG_OK, "Below minimal Log Level", 0);

    Pgs_Log_Error init_err = pgs_log_init_if_needed();
    if (init_err != PGS_LOG_OK) {
        return init_err;
    }

    va_list ap;
    va_start(ap, fmt);

    char msg[PGS_LOG_MAX_ENTRY_LEN];
    int msg_len = vsnprintf(msg, PGS_LOG_MAX_ENTRY_LEN, fmt, ap);
    va_end(ap);

    if (msg_len < 0) {
        return pgs_log_set_last_error(PGS_LOG_ERR, "vsnprintf failed", 0);
    }

    const char *format = PGS_LOG_FORMAT;
    char log_string[PGS_LOG_MAX_ENTRY_LEN];
    size_t pos = 0;

    while (*format && pos < PGS_LOG_MAX_ENTRY_LEN - 1) {
        if (*format == '%' && *(format +1)) {
            switch (*(format + 1)) {
                case 'L': {
                    const char *lvl = pgs_log_level_to_string(level);
                    size_t lvl_length = strlen(lvl);
                    if (pos + lvl_length >= PGS_LOG_MAX_ENTRY_LEN -1) break;
                    memcpy(log_string + pos, lvl, lvl_length);
                    pos += lvl_length;
                    break;
                }
                case 'T': {
                    const char *timestamp = pgs_log_timestamp_string();  // If used more than once per log is less efficient but this way we have most improvement if not used
                    size_t timestamp_length = strlen(timestamp);         // If used more than once per log is less efficient but this way we have most improvement if not used
                    if (pos + timestamp_length >= PGS_LOG_MAX_ENTRY_LEN -1) break;
                    memcpy(log_string + pos, timestamp, timestamp_length);
                    pos += timestamp_length;
                    break;
                }
                case 'F': {
                    if (pos + file_len >= PGS_LOG_MAX_ENTRY_LEN -1) break;
                    memcpy(log_string + pos, file, file_len);
                    pos += file_len;
                    break;
                }
                case 'l': {
                    if (pos + line_len >= PGS_LOG_MAX_ENTRY_LEN -1) break;
                    memcpy(log_string + pos, line, line_len);
                    pos += line_len;
                    break;
                }
                case 'M': {
                    if (pos + msg_len >= PGS_LOG_MAX_ENTRY_LEN -1) break;
                    memcpy(log_string + pos, msg, msg_len);
                    pos += msg_len;
                    break;
                }
                case 'G': {
                    if (tag_len > pgs_max_tag_width)
                        pgs_max_tag_width = tag_len;

                    size_t target_width = pgs_max_tag_width;
                    if (pos + target_width >= PGS_LOG_MAX_ENTRY_LEN - 1) break;

                    memcpy(log_string + pos, tag, tag_len);
                    pos += tag_len;

                    if (tag_len < target_width) {
                        memset(log_string + pos, ' ', target_width - tag_len);
                        pos += (target_width - tag_len);
                    }
                    break;
                }
                default: {
                    if (pos < PGS_LOG_MAX_ENTRY_LEN - 1) log_string[pos++] = format[0];
                    if (pos < PGS_LOG_MAX_ENTRY_LEN - 1) log_string[pos++] = format[1];
                    break;
                }
            }
            format += 2;
        } else {
            log_string[pos++] = *format++;
        }
    }

    log_string[pos++] = '\n';
    log_string[pos] = '\0';

    Pgs_Log_Error err = pgs_log_write_output((const char *)&log_string, pos);
    if (err != PGS_LOG_OK)
        return err;

    return pgs_log_set_last_error(PGS_LOG_OK, "Log entry written", 0);
}

#else // PGS_LOG_ENABLED
Pgs_Log_Error pgs_log(Pgs_Log_Level level, const char *tag, size_t tag_len, const char *file, size_t file_len, const char *line, size_t line_len, const char *fmt, ...) {
    (void)level;
    (void)file;
    (void)file_len;
    (void)line;
    (void)line_len;
    (void)fmt;
    return PGS_LOG_OK;
}

#endif // PGS_LOG_ENABLED

Pgs_Log_Error pgs_log_write_output(const char *str, size_t len) {
    if (!PGS_LOG_ENABLED)
        return pgs_log_set_last_error(PGS_LOG_OK, "Logging disabled", 0);

    for (int i = 0; i < pgs_output_count; ++i) {
        Pgs_Log_Output *o = &pgs_outputs[i];
#if PGS_LOG_ENABLED_BUFFERING
#if PGS_LOG_BUFFER_INSTA_WRITE_TERMINAL
        if (o->fd == stderr || o->fd == stdout) {
            if (pgs_write(fileno(o->fd), str, len) != (ssize_t)len)
                return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to write msg to file", errno);
            continue;
        }
#endif
        if (o->buf_pos + len > PGS_LOG_MAX_OUTPUT_BUFFER_SIZE) {
            if (pgs_write(fileno(o->fd), o->buffer, o->buf_pos) != (ssize_t)o->buf_pos)
                return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to write buffer to file", errno);

            o->buf_pos = 0;

            if (o->buf_pos + len > PGS_LOG_MAX_OUTPUT_BUFFER_SIZE) {
#if PGS_LOG_BUFFER_INSTA_WRITE_IF_TOO_LARGE
                if (pgs_write(fileno(o->fd), str, len) != (ssize_t)len)
                    return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to write msg to file", errno);
#endif
                return pgs_log_set_last_error(PGS_LOG_ERR, "Log file to big for buffering, insta writing, can be disabled with `PGS_LOG_BUFFER_INSTA_WRITE_IF_TOO_LARGE false`", 0);
            }
        }

        memcpy(o->buffer + o->buf_pos, str, len);
        o->buf_pos += len;
#else
        if (pgs_write(fileno(o->fd), str, len) != len)
            return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to write msg to file", errno);
#endif
    }

    return pgs_log_set_last_error(PGS_LOG_OK, "Wrote/Buffered msg to all outputs", 0);
}

Pgs_Log_Error pgs_log_flush(void) {
    for (int i = 0; i < pgs_output_count; ++i) {
        Pgs_Log_Output *o = &pgs_outputs[i];
#if PGS_LOG_ENABLED_BUFFERING
            if (pgs_write(fileno(o->fd), o->buffer, o->buf_pos) != (ssize_t)o->buf_pos)
                return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to write buffer to file", errno);

            o->buf_pos = 0;
#endif
            if (fflush(o->fd) != 0)
                return pgs_log_set_last_error(PGS_LOG_ERR_IO, "Failed to Flush buffer", errno);
    }

    return pgs_log_set_last_error(PGS_LOG_OK, "Flushed all fd's", 0);
}

const char *pgs_log_level_to_string(Pgs_Log_Level level) {
    switch (level) {
        case PGS_LOG_DEBUG: return "DEBUG";
        case PGS_LOG_INFO:  return "INFO ";
        case PGS_LOG_WARN:  return "WARN ";
        case PGS_LOG_ERROR: return "ERROR";
        case PGS_LOG_FATAL: return "FATAL";
        default:            return "UNKWN";
    }
}


const char *pgs_log_timestamp_string(void) {
    time_t current_time = time(NULL);

    if (current_time != pgs_log_last_timestamp) {
        struct tm *tm_info = localtime(&current_time);
        strftime(pgs_log_cached_timestamp, PGS_LOG_MAX_TIMESTAMP_LEN, PGS_LOG_TIMESTAMP_FORMAT, tm_info);
        pgs_log_last_timestamp = current_time;
    }

    return pgs_log_cached_timestamp;
}

Pgs_Log_Error pgs_log_add_fd_output(FILE *file) {
    if (!file)
        return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "No File passed to add to output", 0);

    if (pgs_output_count >= PGS_LOG_MAX_FD) {
        return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "Reached max file descriptor count, you can add `#define PGS_LOG_MAX_FD` and increase the number and recompile", 0);
    }

    Pgs_Log_Output *o = &pgs_outputs[pgs_output_count++];

    o->fd = file;

#if PGS_LOG_ENABLED_BUFFERING
    o->buf_pos = 0;
#endif

    return pgs_log_set_last_error(PGS_LOG_OK, "Added fd to output", 0);
}


Pgs_Log_Error pgs_log_remove_fd_output(FILE *file) {
    if (!file) 
        return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "No file passed", 0);

    int fd_index = -1;

    for (int i = 0; i < pgs_output_count; ++i) {
        Pgs_Log_Output *out = &pgs_outputs[i];
        if (out->fd == file) {
            fd_index = i;
            break;
        };
    }

    if (fd_index == -1) 
        return pgs_log_set_last_error(PGS_LOG_ERR_FILE, "file does not exist in output", 0);

    Pgs_Log_Output *out = &pgs_outputs[fd_index];

    if (out->fd != stdout && out->fd != stderr)
        fclose(out->fd);

    pgs_outputs[fd_index] = pgs_outputs[--pgs_output_count];

    return pgs_log_set_last_error(PGS_LOG_OK, "Removed File from output", 0);
}

Pgs_Log_Error pgs_log_mkdir_if_not_exists(const char *path) {
#ifdef _WIN32
    int result = _mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        if (errno == EEXIST) {
            return pgs_log_set_last_error(PGS_LOG_OK, "Folder does already exist", 0);
        }
        return pgs_log_set_last_error(PGS_LOG_ERR, pgs_log_temp_sprintf("could not create directory `%s`", path), errno);
    }

    return pgs_log_set_last_error(PGS_LOG_OK, "Created Directory", 0);
}

Pgs_Log_Error pgs_log_create_dirs_for_path(const char *fullpath) {
    if (!fullpath)
        return pgs_log_set_last_error(PGS_LOG_ERR, "no path passed", 0);
    size_t len = strlen(fullpath);
    if (len == 0)
        return pgs_log_set_last_error(PGS_LOG_ERR, "Path doesnt have a length", 0);

    if (len >= PGS_LOG_MAX_PATH_LEN) {
        return pgs_log_set_last_error(PGS_LOG_ERR, "Path too long", 0);
    }

    char tmp[PGS_LOG_MAX_PATH_LEN];
    strncpy(tmp, fullpath, PGS_LOG_MAX_PATH_LEN);
    tmp[len] = '\0';

    for (size_t i = 1; i < len; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char orig = tmp[i];
            tmp[i] = '\0';
            if (pgs_log_mkdir_if_not_exists(tmp) != PGS_LOG_OK) {
                return pgs_log_set_last_error(PGS_LOG_ERR, "Failed to create directories", 0);
            }
            tmp[i] = orig;
        }
    }
    return pgs_log_set_last_error(PGS_LOG_OK, pgs_log_temp_sprintf("Created dirs for path %s", fullpath), 0);
}

bool pgs_log_file_exists(const char *file_path) {
    struct stat buffer;
    return (stat(file_path, &buffer) == 0);
}

int pgs_log_get_last_occurence_of(const char ch, const char *string) {
    int last_pos = -1;
    int pos = 0;
    while (*(string + pos)) {
        if (*(string + pos) == ch) {
            last_pos = pos;
        }
        pos += 1;
    }
    return last_pos;
}

void pgs_log_cleanup(void) {
    if (pgs_log_flush() != PGS_LOG_OK)
        pgs_log_print_error_detail();

    for (int i = 0; i < pgs_output_count; ++i) {
        Pgs_Log_Output *out = &pgs_outputs[i];
        if (!out->fd) continue;
        if (out->fd == stdout || out->fd == stderr) continue;

        fclose(out->fd);
        out->fd = NULL;
    }
    pgs_output_count = 0;
    pgs_log_initialized = false;
}

void pgs_log_toggle(bool enabled) {
    pgs_log_is_enabled = enabled;
}

void pgs_log_print_error_detail(void) {
#if PGS_LOG_USE_DETAIL_ERROR
    const char *type_str = "UNKNOWN";
    switch (pgs_log_last_error.type) {
        case PGS_LOG_OK:        type_str = "OK"; break;
        case PGS_LOG_ERR:       type_str = "ERROR"; break;
        case PGS_LOG_ERR_FILE:  type_str = "FILE ERROR"; break;
        case PGS_LOG_ERR_IO:    type_str = "IO ERROR"; break;
    }

    fprintf(stderr, "==================== PGS LOG ERROR ====================\n");
    fprintf(stderr, "Type      : %s\n", type_str);
    fprintf(stderr, "Errno     : %d ", pgs_log_last_error.errno_value);
    if (pgs_log_last_error.errno_value != 0) {
        fprintf(stderr, " (%s)\n", strerror(pgs_log_last_error.errno_value));
    } else {
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "Message   : %s\n", pgs_log_last_error.message[0] ? pgs_log_last_error.message : "(none)");
    fprintf(stderr, "=======================================================\n");
#else
    fprintf(stderr, "PGS_LOG: detailed error reporting is disabled at compile time.\n");
#endif
}

static char pgs_internal_temp_buffers[PGS_LOG_TEMP_BUFFERS][PGS_LOG_MAX_TEMP_BUFFER_LEN];
static int pgs_temp_id = 0;

char *pgs_log_temp_sprintf(const char *format, ...) {
    char *buffer = pgs_internal_temp_buffers[pgs_temp_id];
    pgs_temp_id = (pgs_temp_id + 1) % PGS_LOG_TEMP_BUFFERS;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, PGS_LOG_MAX_TEMP_BUFFER_LEN, format, args);
    va_end(args);

    return buffer;
}


#endif // PGS_LOG_IMPLEMENTATION

#ifndef PGS_LOG_STRIP_PREFIX_GUARD_
#define PGS_LOG_STRIP_PREFIX_GUARD_

    #ifdef PGS_LOG_STRIP_PREFIX

        #define log pgs_log
        #define level_to_string pgs_log_level_to_string
        #define timestamp_string pgs_log_timestamp_string
        #define add_fd_output pgs_log_add_fd_output
        #define remove_fd_output pgs_log_remove_fd_output
        #define mkdir_if_not_exists pgs_log_mkdir_if_not_exists
        #define create_dirs_for_path pgs_log_create_dirs_for_path
        #define file_exists pgs_log_file_exists
        #define get_last_occurence_of pgs_log_get_last_occurence_of
        #define log_toggle pgs_log_toggle
        #define print_error_detail pgs_log_print_error_detail
        #define temp_sprintf pgs_log_temp_sprintf
        #define get_last_error pgs_log_get_last_error
        #define set_last_error pgs_log_set_last_error
        #define cleanup pgs_log_cleanup
        #define write_output pgs_log_write_output
        #define flush pgs_log_flush


        #define LOG_DEBUG PGS_LOG_DEBUG
        #define LOG_INFO PGS_LOG_INFO
        #define LOG_WARN PGS_LOG_WARN
        #define LOG_ERROR PGS_LOG_ERROR
        #define LOG_FATAL PGS_LOG_FATAL

        #define Log_Level Pgs_Log_Level
        #define Log_Error Pgs_Log_Error
        #define Log_Error_Detail Pgs_Log_Error_Detail
        #define Log_Output Pgs_Log_Output

        #define minimal_log_level pgs_log_minimal_log_level

    #endif // PGS_LOG_STRIP_PREFIX

#endif // PGS_LOG_STRIP_PREFIX_GUARD_

/*
    Revision History:

        0.4.5 (2026-05-14) Remove from binary if PGS_LOG_ENABLED=false + default enable stdout

        0.4.4 (2025-09-29) Bug fixes + SigInt handler

        0.4.3 (2025-09-27) Performance Improvements
                            - less formatting/write overhead, timestamp caching, larger buffers, less strlen
                            - went from around 1.2s for 1M logs to 45-60ms based if using static logs or dynamic
                            - tested on arch with ryzen 7 7700X
                            - marco usage stays the same, direct calls to pgs_log need to be adjusted

        0.4.2 (2025-09-19) Bugfix
                            - fixed wrong struct if not using detailed error mode
                            - removed warnings

        0.4.1 (2025-09-18) Bugfix
                            - fixed bug using old/non exisitng variable in log if not appending and not overriding

        0.4.0 (2025-09-18) Add Log buffering
                            - configurable log buffering for better performance
                            - option to insta write to stderr/stdout

        0.3.1 (2025-09-17) Cap File creation amount

        0.3.0 (2025-09-17) Improved Memory safety and buffer handling
                            - no more dynamic allocations, no malloc
                            - no more hardcoded sites, all configurable via #define
                            - safe functions only (strncpy, vsnprintf)
                            - better temp buffer (multiple and again definable sizes)

        0.2.0 (2025-09-17) Extended Error Handling
                            - Added Revision History and introduction text at top
                            - Add Pgs_Log_Error enum for error codes
                            - Add Pgs_Log_Error_Detail for better reports
                            - add toggle for enabling logging
                            - improve error messages/handling
                            - change return typesof most functions from bool to Pgs_Log_Erro
                            - change to dual license (add public domain)
                            - fix naming inconsistencies
                            Breaking: bool style checks not possible anymore, if caring about about return type must check again PGS_LOG_OK

        0.1.0 (2025-09-14) Initial Release
                            - Basic Logging to stdout/stderr/file
                            - Auto Log file creation
                            - custom log formats
                            - custom log file formats
                            - Log Levels
                            - setting min log level
*/


/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025 Paul Geisthardt
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
