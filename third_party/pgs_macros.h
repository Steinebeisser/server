/* PGS_MACROS - v0.1.0 - Public Domain - https://github.com/Steinebeisser/pgs/blob/master/pgs_macros.h
 *
 * simple macro collection that can be useful in almost every project
 *
 */

#ifndef PGS_MACROS_H
#define PGS_MACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>

#ifndef PGS_IGNORE_STATIC_ASSERT
#   if defined (__cplusplus)
#      if __cplusplus >= 201103L
#          define PGS_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#      else
#          error "PGS_STATIC_ASSERT requires c++11 or newer, please define PGS_IGNORE_STATIC_ASSERT to compile for older versions"
#      endif
#   elif defined (__STDC_VERSION__)
#      if __STDC_VERSION__ >= 202311L // C23
#          define PGS_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
#      elif __STDC_VERSION__ >= 201112L // C11 - C17
#          define PGS_STATIC_ASSERT(expr, msg) _Static_assert(expr, msg)
#      else
#          error "Static assert exists only since C11, please define PGS_IGNORE_STATIC_ASSERT to compile for older versions"
#      endif
#   else
#      error "PGS_STATIC_ASSERT requires C11 or C++11 and newer, please define PGS_IGNORE_STATIC_ASSERT to compile for older versions"
#   endif
#endif // PGS_IGNORE_STATIC_ASSERT

#if defined(__GNUC__) || defined(__clang__)

#   define PGS_NORETURN                           __attribute__((noreturn))
#   define PGS_DEPRECATED(msg)                    __attribute__((deprecated(msg)))
#   define PGS_PRINTF_LIKE(fmt_index, first_arg)  __attribute__((format(printf, fmt_index, first_arg)))
#   define PGS_FORCE_INLINE                       static inline __attribute__((always_inline))
#   define PGS_NOINLINE                           __attribute__((noinline))
#   define PGS_COLD                               __attribute__((cold))
#   define PGS_HOT                                __attribute__((hot))

#   define PGS_LIKELY(x)                          __builtin_expect(!!(x), 1)
#   define PGS_UNLIKELY(x)                        __builtin_expect(!!(x), 0)
#   define PGS_COMPILER_UNREACHABLE()             __builtin_unreachable()
#   define PGS_DEBUG_BREAK()                      __builtin_trap()
#   define PGS_UNUSED                             __attribute__((unused))
#   define PGS_WARN_UNUSED_RESULT                 __attribute__((warn_unused_result))

#elif defined(_MSC_VER)

#   define PGS_NORETURN                           __declspec(noreturn)
#   define PGS_DEPRECATED(msg)                    __declspec(deprecated(msg))
#   define PGS_PRINTF_LIKE(fmt_index, first_arg)
#   define PGS_FORCE_INLINE                       static __forceinline
#   define PGS_NOINLINE                           __declspec(noinline)
#   define PGS_COLD
#   define PGS_HOT

#   define PGS_LIKELY(x)                          (x)
#   define PGS_UNLIKELY(x)                        (x)
#   define PGS_COMPILER_UNREACHABLE()             __assume(0)
#   define PGS_DEBUG_BREAK()                      __debugbreak()
#   define PGS_UNUSED
#   define PGS_WARN_UNUSED_RESULT                 [[nodiscard]]
#else
#   define PGS_NORETURN
#   define PGS_DEPRECATED(msg)
#   define PGS_PRINTF_LIKE(fmt_index, first_arg)
#   define PGS_FORCE_INLINE                       static inline
#   define PGS_NOINLINE
#   define PGS_COLD
#   define PGS_HOT

#   define PGS_LIKELY(x)                          (x)
#   define PGS_UNLIKELY(x)                        (x)
#   define PGS_COMPILER_UNREACHABLE()             ((void)0)
#   define PGS_DEBUG_BREAK()                      abort()
#   define PGS_UNUSED
#   define PGS_WARN_UNUSED_RESULT
#endif


/**
 * release + debug mode checks, for debug only use PGS_ASSERT
 */
#define PGS_CHECK(expr)                                     \
        do {                                                \
            if (PGS_UNLIKELY(!(expr))) {                    \
                PGS_PANIC("assertion failed: %s", #expr);   \
            }                                               \
        } while(0)

// release + debug mode checks, for debug only use PGS_ASSERT_MSG
#define PGS_CHECK_MSG(expr, ...)                            \
        do {                                                \
            if (PGS_UNLIKELY(!(expr))) {                    \
                PGS_PANIC(__VA_ARGS__);                     \
            }                                               \
        } while(0)

#ifndef NDEBUG
// debug mode only checks, for debug + release checks use PGS_CHECK
#   define PGS_ASSERT(expr)                                 \
        do {                                                \
            if (PGS_UNLIKELY(!(expr))) {                    \
                PGS_PANIC("assertion failed: %s", #expr);   \
            }                                               \
        } while(0)

// debug mode only checks, for debug + release checks use PGS_CHECK_MSG
#define PGS_ASSERT_MSG(expr, ...)                           \
        do {                                                \
            if (PGS_UNLIKELY(!(expr))) {                    \
                PGS_PANIC(__VA_ARGS__);                     \
            }                                               \
        } while(0)
#else
#   define PGS_ASSERT(expr)          ((void)0)
#   define PGS_ASSERT_MSG(expr, ...) ((void)0)
#endif

#define PGS_STRINGIFY_(x) #x
#define PGS_STRINGIFY(x)  PGS_STRINGIFY_(x)

#define PGS_CONCAT_(a, b) a##b
#define PGS_CONCAT(a, b)  PGS_CONCAT_(a, b)

#define PGS_TODO(msg) \
    do {                                                               \
        fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, msg); \
        abort();                                                       \
    } while (0)


#define PGS_UNREACHABLE(msg)                                                  \
    do {                                                                      \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, msg); \
        abort();                                                              \
        PGS_COMPILER_UNREACHABLE();                                           \
    } while(0)

#define PGS_ARRAY_LEN(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

PGS_COLD PGS_NORETURN void pgs_panic(const char *file, int line, const char *fmt, ...) PGS_PRINTF_LIKE(3, 4);

// PANICS
#define PGS_PANIC(fmt, ...) \
    pgs_panic(__FILE__, __LINE__, fmt, ##__VA_ARGS__)


#ifdef __cplusplus
#define PGS_KIB(x) (static_cast<size_t>(x) * 1024ULL)
#define PGS_KB(x)  (static_cast<size_t>(x) * 1000ULL)
#else
#define PGS_KIB(x) ((size_t)(x) * 1024ULL)
#define PGS_KB(x)  ((size_t)(x) * 1000ULL)
#endif // __cplusplus

#define PGS_MIB(x) (PGS_KIB(x)  * 1024ULL)
#define PGS_GIB(x) (PGS_MIB(x)  * 1024ULL)
#define PGS_TIB(x) (PGS_GIB(x)  * 1024ULL)

#define PGS_MB(x)  (PGS_KB(x)   * 1000ULL)
#define PGS_GB(x)  (PGS_MB(x)   * 1000ULL)
#define PGS_TB(x)  (PGS_GB(x)   * 1000ULL)

#endif //  PGS_MACROS_H

#ifdef PGS_MACROS_IMPLEMENTATION

PGS_NORETURN void pgs_panic(const char *file, int line, const char *fmt, ...) {
#ifdef PGS_PANIC_FUNNY
    PGS_TODO("Funny panic");
#else
    fprintf(stderr, "%s:%d: PANIC: ", file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    abort();
#endif
}

#endif // PGS_MACROS_IMPLEMENTATION

#ifdef PGS_MACROS_STRIP_PREFIX

#   define TODO                  PGS_TODO
#   define UNREACHABLE           PGS_UNREACHABLE
#   define UNUSED                PGS_UNUSED
#   define NORETURN              PGS_NORETURN
#   define DEPRECATED            PGS_DEPRECATED
#   define PRINTF_LIKE           PGS_PRINTF_LIKE
#   define FORCE_INLINE          PGS_FORCE_INLINE
#   define NOINLINE              PGS_NOINLINE
#   define COLD                  PGS_COLD
#   define HOT                   PGS_HOT
#   define LIKELY                PGS_LIKELY
#   define UNLIKELY              PGS_UNLIKELY
#   define COMPILER_UNREACHABLE  PGS_COMPILER_UNREACHABLE
#   define DEBUG_BREAK           PGS_DEBUG_BREAK
#   define CHECK                 PGS_CHECK
#   define CHECK_MSG             PGS_CHECK_MSG
#   define ASSERT                PGS_ASSERT
#   define ASSERT_MSG            PGS_ASSERT_MSG
#   define STRINGIFY             PGS_STRINGIFY
#   define CONCAT                PGS_CONCAT
#   define ARRAY_LEN             PGS_ARRAY_LEN
#   define PANIC                 PGS_PANIC
#   define KIB                   PGS_KIB
#   define MIB                   PGS_MIB
#   define GIB                   PGS_GIB
#   define TIB                   PGS_TIB
#   define KB                    PGS_KB
#   define MB                    PGS_MB
#   define GB                    PGS_GB
#   define TB                    PGS_TB

#   ifndef PGS_IGNORE_STATIC_ASSERT
#       define STATIC_ASSERT         PGS_STATIC_ASSERT
#   endif // PGS_IGNORE_STATIC_ASSERT

#endif // PGS_MACROS_STRIP_PREFIX

/*
    Revision History:

        0.1.0 (2026-05-11) Initial public release
*/


/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2026 Paul Geisthardt
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
