/* PGS_ARGS - v0.3.1 - Public Domain - https://github.com/Steinebeisser/pgs/blob/master/pgs_args.h
 *
 * USAGE:
 * Define PGS_ARGS macro with your arguments before including this header.
 * Each argument is defined as: PGS_ARG(type, name, short_flag, long_flag, description, valid_values)
 *
 * ARGUMENT TYPES:
 *   - PGS_ARG_FLAG: Boolean flag (no value)
 *       Usage: -v, --verbose
 *       Sets name_present=true, name_value="1"
 *
 *   - PGS_ARG_VALUE: Requires a value
 *       Short form: -o file.txt  OR  -ofile.txt
 *       Long form:  --output file.txt
 *       Sets name_present=true, name_value="file.txt"
 *
 *   - PGS_ARG_OPTIONAL: Optional value (must use prefix syntax if provided)
 *       Short form without value: -t              -> name_value=NULL
 *       Short form with value:    -t=file.txt     -> name_value="file.txt"
 *                                 -tfile.txt       -> name_value="file.txt"
 *                                 -t:file.txt      -> name_value="file.txt"
 *       Long form without value:  --dump-tokens   -> name_value=NULL
 *       Long form with value:     --dump-tokens=file.txt  -> name_value="file.txt"
 *                                 --dump-tokens:file.txt  -> name_value="file.txt"
 *
 *       Note: For optional args, the value MUST be attached/prefixed. Arguments like
 *       "-t nextarg" will treat "nextarg" as a positional argument, not as the value for -t.
 *       This prevents optional args from accidentally consuming positional arguments.
 *
 * valid_values: Pipe-separated list of valid values (e.g., "0|1|2|3|s" for optimization levels)
 *               Use NULL for no validation
 *
 * EXAMPLE:
 * #define PGS_ARGS \
 *     PGS_ARG(PGS_ARG_OPTIONAL, help, 'h', "help", "Show help message", NULL) \
 *     PGS_ARG(PGS_ARG_FLAG, verbose, 'v', "verbose", "Enable verbose output", NULL) \
 *     PGS_ARG(PGS_ARG_VALUE, output, 'o', "output", "Output file", NULL) \
 *     PGS_ARG(PGS_ARG_VALUE, optimize, 'O', "optimize", "Optimization level", "0|1|2|3|s")
 *
 * USAGE EXAMPLES:
 *   program -v file.txt                  # verbose flag, file.txt is positional
 *   program -o output.txt file.txt       # output value, file.txt is positional
 *   program -h topic file.txt            # help has no value, "topic" and "file.txt" are positional
 *   program -h=topic file.txt            # help value is "topic", file.txt is positional
 *   program --help=advanced              # help value is "advanced"
 */
#pragma GCC system_header
#if defined(__GNUC__) || defined(__clang__)
#   define PGS_ARGS_UNUSED __attribute__((unused))
#else
#   define PGS_ARGS_UNUSED
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#ifndef PGS_ARGS_FUNC_PREFIX
#   define PGS_ARGS_FUNC_PREFIX pgs_args
#endif

#define PGS__PASTE_(a,b) a##b
#define PGS__PASTE(a,b)  PGS__PASTE_(a,b)
#define PGS__FN(name)    PGS__PASTE(PGS_ARGS_FUNC_PREFIX, name)
#define PGS__ARGS_T      PGS__PASTE(PGS_ARGS_FUNC_PREFIX, _t)
#define PGS__ID          PGS__PASTE(PGS_ARGS_FUNC_PREFIX, _arg_id)
#define PGS__META        PGS__PASTE(PGS_ARGS_FUNC_PREFIX, _arg_meta)
#define PGS__COUNT       PGS__PASTE(PGS_ARGS_FUNC_PREFIX, _ARG_COUNT)

#define KIB (1024.0)
#define MIB (KIB * 1024.0)
#define GIB (MIB * 1024.0)
#define TIB (GIB * 1024.0)

#define KB  (1000.0)
#define MB  (KB * 1000.0)
#define GB  (MB * 1000.0)
#define TB  (GB * 1000.0)

#define PGS_ARGS_INT       "$int"
#define PGS_ARGS_UINT      "$uint"
#define PGS_ARGS_POSINT    "$posint"
#define PGS_ARGS_NUMBER    "$number"
#define PGS_ARGS_BOOL      "$bool"

typedef enum {
    PGS_ARG_FLAG,
    PGS_ARG_VALUE,
    PGS_ARG_OPTIONAL,
    PGS_ARG_SUBCOMMAND
} PgsArgType;

typedef struct {
#define PGS_ARG(arg_type, name, short_flag, long_flag, description, valid_values) \
    bool name##_present; \
    const char *name##_value;
PGS_ARGS
#undef PGS_ARG

    const char **positionals;
    int positional_count;
} PGS__ARGS_T;

typedef enum {
#define PGS_ARG(arg_type, name, short_flag, long_flag, description, valid_values) \
    PGS__PASTE(PGS__ID, _##name),
PGS_ARGS
#undef PGS_ARG
    PGS__COUNT
} PGS__ID;

typedef struct {
    const char *name;
    char short_flag;
    const char *long_flag;
    const char *description;
    const char *valid_values;
    PgsArgType type;
} PgsArgMeta;

static const PgsArgMeta PGS__META[] = {
#define PGS_ARG(arg_type, name, short_flag, long_flag, description, valid_values) \
    { #name, short_flag, long_flag, description, valid_values, arg_type },
PGS_ARGS
#undef PGS_ARG
};

bool PGS__FN(_parse)(PGS__ARGS_T *args, int argc, char** argv, bool ignore_on_error);
void PGS__FN(_print_help)(void);
void PGS__FN(_print_help_specific_id)(PGS__ID arg_id);
void PGS__FN(_print_help_specific_name)(const char *name);


static uint64_t PGS_ARGS_UNUSED pgs_args_parse_count(const char *str);
static uint64_t PGS_ARGS_UNUSED pgs_args_parse_size(const char *str);
static bool     PGS_ARGS_UNUSED pgs_args_parse_bool(const char *str);

#ifdef PGS_ARGS_IMPLEMENTATION

static const char *PGS__FN(_builtin_help)(const char *validator) {
    if (strcmp(validator, PGS_ARGS_INT) == 0)
        return "signed integer, e.g. -5, 0, 5";

    if (strcmp(validator, PGS_ARGS_UINT) == 0)
        return "unsigned integer, 0 and up";

    if (strcmp(validator, PGS_ARGS_POSINT) == 0)
        return "positive integer, 1 and up";

    if (strcmp(validator, PGS_ARGS_NUMBER) == 0)
        return "number, including floats";

    if (strcmp(validator, PGS_ARGS_BOOL) == 0)
        return "bool, 1/0, true/false";

    return NULL;
}

static void PGS__FN(_print_valid_values_error)(const char *valid_values) {
    if (!valid_values) return;

    char *dup = strdup(valid_values);
    if (!dup) {
        fprintf(stderr, "%s", valid_values);
        return;
    }

    char *token = strtok(dup, "|");
    bool first = true;

    while (token) {
        const char *help = PGS__FN(_builtin_help)(token);

        if (!first)
            fprintf(stderr, ", ");

        if (help)
            fprintf(stderr, "%s (%s)", token, help);
        else
            fprintf(stderr, "%s", token);

        first = false;
        token = strtok(NULL, "|");
    }

    free(dup);
}

static bool PGS__FN(_is_int)(const char *s) {
    if (!s || !*s) return false;

    errno = 0;
    char *end = NULL;
    strtoll(s, &end, 10);

    return errno != ERANGE && end != s && *end == '\0';
}

static bool PGS__FN(_is_uint)(const char *s) {
    if (!s || !*s) return false;
    if (*s == '-') return false;

    errno = 0;
    char *end = NULL;
    strtoull(s, &end, 10);

    return errno != ERANGE && end != s && *end == '\0';
}

static bool PGS__FN(_is_posint)(const char *s) {
    if (!PGS__FN(_is_uint)(s)) return false;

    char *end = NULL;
    unsigned long long n = strtoull(s, &end, 10);

    return n > 0;
}

static bool PGS__FN(_is_number)(const char *s) {
    if (!s || !*s) return false;

    errno = 0;
    char *end = NULL;
    strtod(s, &end);

    return errno != ERANGE && end != s && *end == '\0';
}

static bool PGS__FN(_is_bool)(const char *s) {
    if (!s || !*s) return false;

    while (isspace((unsigned char)*s))
        s++;

    if (strcmp(s, "1") == 0 || strcmp(s, "0") == 0) return true;

    const char *t = "true";
    const char *f = "false";

    size_t len = strlen(s);
    if (len == 4) {
        for (size_t i = 0; i < len; ++i) {
            if (tolower(s[i]) != t[i]) return false;
        }

        return true;
    }

    if (len == 5) {
        for (size_t i = 0; i < len; ++i) {
            if (tolower(s[i]) != f[i]) return false;
        }

        return true;
    }

    return false;
}

static bool PGS__FN(_validate_builtin)(const char *rule, const char *value) {
    if (strcmp(rule, PGS_ARGS_INT) == 0)
        return PGS__FN(_is_int)(value);

    if (strcmp(rule, PGS_ARGS_UINT) == 0)
        return PGS__FN(_is_uint)(value);

    if (strcmp(rule, PGS_ARGS_POSINT) == 0)
        return PGS__FN(_is_posint)(value);

    if (strcmp(rule, PGS_ARGS_NUMBER) == 0)
        return PGS__FN(_is_number)(value);

    if (strcmp(rule, PGS_ARGS_BOOL) == 0)
        return PGS__FN(_is_bool)(value);

    return false;
}

static uint64_t pgs_args_parse_size(const char *str) {
    if (!str) return 0;

    char *end;
    double val = strtod(str, &end);
    if (end == str) return 0;

    while (*end == ' ') end++;

    if      (!strcasecmp(end, "kib")) val *= KIB;
    else if (!strcasecmp(end, "mib")) val *= MIB;
    else if (!strcasecmp(end, "gib")) val *= GIB;
    else if (!strcasecmp(end, "tib")) val *= TIB;
    else if (!strcasecmp(end, "kb") || !strcasecmp(end, "k")) val *= KB;
    else if (!strcasecmp(end, "mb") || !strcasecmp(end, "m")) val *= MB;
    else if (!strcasecmp(end, "gb") || !strcasecmp(end, "g")) val *= GB;
    else if (!strcasecmp(end, "tb") || !strcasecmp(end, "t")) val *= TB;

    return (uint64_t)val;
}

static uint64_t pgs_args_parse_count(const char *str) {
    if (!str || !*str)
        return 0;

    while (isspace((unsigned char)*str))
        str++;

    errno = 0;
    char *end = NULL;
    unsigned long long n = strtoull(str, &end, 10);

    if (errno == ERANGE)
        return 0;

    if (end == str || !end || *end != '\0')
        return 0;

    return n;
}

static bool PGS_ARGS_UNUSED pgs_args_parse_bool(const char *str) {
    if (!str || !*str) return false;

    while (isspace((unsigned char)*str))
        str++;

    if (strcmp(str, "1") == 0) return true;

    if (strcmp(str, "0") == 0) return false;

    const char *true_str = "true";
    const char *false_str = "false";

    size_t len = strlen(str);

    if (len == 4) {
        for (size_t i = 0; i < 4; i++) {
            if (tolower((unsigned char)str[i]) != true_str[i])
                return false;
        }

        return true;
    }

    if (len == 5) {
        for (size_t i = 0; i < 5; i++) {
            if (tolower((unsigned char)str[i]) != false_str[i])
                return false;
        }

        return false;
    }

    return false;
}

static PGS__ID PGS__FN(_find_by_short)(char flag) {
    for (int i = 0; i < PGS__COUNT; ++i) {
        if (PGS__META[i].short_flag == flag) {
            return (PGS__ID)i;
        }
    }
    return PGS__COUNT;
}

static PGS__ID PGS__FN(_find_by_long)(const char *flag) {
    for (int i = 0; i < PGS__COUNT; ++i) {
        if (PGS__META[i].long_flag && strcmp(PGS__META[i].long_flag, flag) == 0) {
            return (PGS__ID)i;
        }
    }
    return PGS__COUNT;
}

static PGS__ID PGS__FN(_find_by_name)(const char *name) {
    for (int i = 0; i < PGS__COUNT; ++i) {
        if (strcmp(PGS__META[i].name, name) == 0) {
            return (PGS__ID)i;
        }
    }
    return PGS__COUNT;
}

bool PGS__FN(_validate_value)(PGS__ID arg_id, const char *value) {
    if (arg_id >= PGS__COUNT) return false;

    const char *valid_values = PGS__META[arg_id].valid_values;
    if (!valid_values || !value) return true;

    char *valid_copy = strdup(valid_values);
    char *token = strtok(valid_copy, "|");

    while (token) {
        if (strcmp(token, value) == 0 || PGS__FN(_validate_builtin)(token, value)) {
            free(valid_copy);
            return true;
        }
        token = strtok(NULL, "|");
    }

    free(valid_copy);
    return false;
}

static void PGS__FN(_set_value)(PGS__ARGS_T *args, PGS__ID arg_id, const char *value) {
    switch (arg_id) {
#define PGS_ARG(arg_type, name, short_flag, long_flag, description, valid_values) \
        case PGS__PASTE(PGS__ID, _##name): \
            args->name##_present = true; \
            args->name##_value = value; \
            break;
        PGS_ARGS
#undef PGS_ARG
        case PGS__COUNT:
            break;
        default: break;
    }
}

bool PGS__FN(_parse)(PGS__ARGS_T *args, int argc, char** argv, bool ignore_on_error) {
    if (!args || !argv || argc < 0) return false;

    memset(args, 0, sizeof(PGS__ARGS_T));
    args->positionals = NULL;

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        if (arg[0] == '-' && arg[1] == '-') {
            const char *flag_name = arg + 2;

            const char *eq_pos = strchr(flag_name, '=');
            const char *colon_pos = strchr(flag_name, ':');
            const char *sep_pos = NULL;

            if (eq_pos && (!colon_pos || eq_pos < colon_pos)) {
                sep_pos = eq_pos;
            } else if (colon_pos) {
                sep_pos = colon_pos;
            }

            char flag_name_only[256];
            if (sep_pos) {
                size_t len = sep_pos - flag_name;
                if (len >= sizeof(flag_name_only)) len = sizeof(flag_name_only) - 1;
                strncpy(flag_name_only, flag_name, len);
                flag_name_only[len] = '\0';
                flag_name = flag_name_only;
            }

            PGS__ID arg_id = PGS__FN(_find_by_long)(flag_name);

            if (arg_id == PGS__COUNT) {
                if (ignore_on_error)
                    continue;
                fprintf(stderr, "Error: Unknown argument '--%s'\n", flag_name);
                return false;
            }

            const PgsArgMeta *meta = &PGS__META[arg_id];

            if (meta->type == PGS_ARG_FLAG) {
                PGS__FN(_set_value)(args, arg_id, "1");
            } else if (meta->type == PGS_ARG_VALUE) {
                if (i + 1 >= argc) {
                    if (ignore_on_error)
                        continue;
                    fprintf(stderr, "Error: Argument '--%s' requires a value\n", flag_name);
                    return false;
                }
                const char *value = argv[++i];
                if (!PGS__FN(_validate_value)(arg_id, value)) {
                    if (ignore_on_error)
                        continue;
                    fprintf(stderr, "Error: Invalid value '%s' for '--%s'. Valid values: ",
                            value, flag_name);
                    PGS__FN(_print_valid_values_error)(meta->valid_values);
                    fprintf(stderr, "\n");
                    return false;
                }
                PGS__FN(_set_value)(args, arg_id, value);
            } else if (meta->type == PGS_ARG_OPTIONAL) {
                const char *value = sep_pos ? (sep_pos + 1) : NULL;

                if (!value && i + 1 < argc && argv[i + 1][0] != '-') {
                    value = argv[++i];
                }

                if (value && !PGS__FN(_validate_value)(arg_id, value)) {
                    if (ignore_on_error)
                        continue;
                    fprintf(stderr, "Error: Invalid value '%s' for '--%s'. Valid values: ",
                            value, flag_name);
                    PGS__FN(_print_valid_values_error)(meta->valid_values);
                    fprintf(stderr, "\n");
                    return false;
                }

                PGS__FN(_set_value)(args, arg_id, value);
            }
        }
        else if (arg[0] == '-' && arg[1] != '\0') {
            for (const char *f = arg + 1; *f; ++f) {
                PGS__ID arg_id = PGS__FN(_find_by_short)(*f);
                if (arg_id == PGS__COUNT) {
                    if (ignore_on_error)
                        continue;
                    fprintf(stderr, "Error: Unknown argument '-%c'\n", *f);
                    return false;
                }
                const PgsArgMeta *meta = &PGS__META[arg_id];

                if (meta->type == PGS_ARG_FLAG) {
                    PGS__FN(_set_value)(args, arg_id, "1");
                } else if (meta->type == PGS_ARG_OPTIONAL) {
                    const char *value = NULL;
                    if (f[1] == '=' || f[1] == ':') {
                        value = f + 2;
                    } else if (f[1] != '\0') {
                        value = f + 1;
                    } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                        value = argv[++i];
                    }
                    PGS__FN(_set_value)(args, arg_id, value);
                    break;
                } else {
                    const char *value = (f[1] != '\0') ? (f + 1) : NULL;
                    if (!value) {
                        if (i + 1 >= argc) {
                            if (ignore_on_error)
                                continue;
                            fprintf(stderr, "Error: Argument '-%c' requires a value\n", *f);
                            return false;
                        }
                        value = argv[++i];
                    }
                    if (value && !PGS__FN(_validate_value)(arg_id, value)) {
                        if (ignore_on_error)
                            continue;
                        fprintf(stderr, "Error: Invalid value '%s' for '-%c'. Valid values: ", value, *f);
                        PGS__FN(_print_valid_values_error)(meta->valid_values);
                        fprintf(stderr, "\n");
                        return false;
                    }
                    PGS__FN(_set_value)(args, arg_id, value);
                    break;
                }
            }
        }
        else {
            PGS__ID arg_id = PGS__FN(_find_by_name)(arg);

            if (arg_id != PGS__COUNT && PGS__META[arg_id].type == PGS_ARG_SUBCOMMAND) {
                const char *value = argv[++i];

                PGS__FN(_set_value)(args, arg_id, value);

                return true;
            } else {
                void *new_positionals = realloc(args->positionals, sizeof(*args->positionals) * (args->positional_count + 1));
                if (!new_positionals) {
                    if (ignore_on_error)
                        continue;
                    fprintf(stderr, "Error: Out of memory\n");
                    return false;
                }
                args->positionals = (const char **)new_positionals;
                args->positionals[args->positional_count++] = arg;
            }
        }
    }

    return true;
}

static void print_valid_values_pretty(const char *s) {
    if (!s) return;

    char *dup = strdup(s);
    printf("      Valid values:\n");
    char* token = strtok(dup, "|");

    while (token != NULL) {
        const char *internal_help = PGS__FN(_builtin_help)(token);
        if (internal_help) {
            printf("        - %s\n", internal_help);
        } else {
            printf("        - %s\n", token);
        }
        token = strtok(NULL, "|");
    }

    free(dup);
}

void PGS__FN(_print_help)(void) {
    printf("Available arguments:\n\n");

    for (int i = 0; i < PGS__COUNT; ++i) {
        const PgsArgMeta *meta = &PGS__META[i];

        printf("  ");
        if (meta->type == PGS_ARG_SUBCOMMAND) {
            printf("%s", meta->name);
        } else {
            if (meta->short_flag) {
                printf("-%c", meta->short_flag);
                if (meta->long_flag) printf(", ");
            }
            if (meta->long_flag) {
                printf("--%s", meta->long_flag);
            }
        }

        printf("\n      ");

        size_t desc_len = strlen(meta->description);
        for (size_t i = 0; i < desc_len; ++i) {
            const char c = meta->description[i];
            if (c == '\n') {
                printf("\n      ");
                continue;
            }

            printf("%c", c);
        }
        printf("\n");

        if (meta->valid_values) {
            print_valid_values_pretty(meta->valid_values);
            // printf("      Valid values: %s\n", meta->valid_values);
        }

        printf("\n");
    }
}

void PGS__FN(_print_help_specific_name)(const char *name) {
    PGS__ID arg_id = PGS__FN(_find_by_name)(name);
    if (arg_id == PGS__COUNT) {
        fprintf(stderr, "Failed to find argument for `%s`\n", name);
        return;
    }

    PGS__FN(_print_help_specific_id)(arg_id);
}

void PGS__FN(_print_help_specific_id)(PGS__ID arg_id) {

    if (arg_id == PGS__COUNT) {
        fprintf(stderr, "Error: Unknown argument");
        PGS__FN(_print_help)();
        return;
    }

    const PgsArgMeta *meta = &PGS__META[arg_id];

    printf("Help for '%s':\n\n", meta->name);
    printf("  ");
    if (meta->short_flag) {
        printf("-%c", meta->short_flag);
        if (meta->long_flag) printf(", ");
    }
    if (meta->long_flag) {
        printf("--%s", meta->long_flag);
    }
    printf("\n\n");

    printf("  %s\n\n", meta->description);

    if (meta->valid_values) {
        printf("  Valid values: %s\n", meta->valid_values);
    }

    const char *type_str = "unknown";
    switch (meta->type) {
        case PGS_ARG_FLAG: type_str = "flag (no value needed)"; break;
        case PGS_ARG_VALUE: type_str = "requires value"; break;
        case PGS_ARG_OPTIONAL: type_str = "optional value"; break;
        case PGS_ARG_SUBCOMMAND: type_str = "subcommand value"; break;
    }
    printf("  Type: %s\n", type_str);
}

#endif // PGS_ARGS_IMPLEMENTATION

#undef PGS_ARGS_FUNC_PREFIX
#undef PGS__PASTE_
#undef PGS__PASTE
#undef PGS__FN
#undef PGS__ARGS_T
#undef PGS__ID
#undef PGS__META
#undef PGS__COUNT
#undef PGS_ARGS
#undef PGS_ARGS_IMPLEMENTATION

#ifndef PGS_ARGS_STRIP_PREFIX_GUARD_
#define PGS_ARGS_STRIP_PREFIX_GUARD_

    #ifdef PGS_ARGS_STRIP_PREFIX

        #define args_parse pgs_args_parse
        #define args_print_help pgs_args_print_help
        #define args_print_help_specific_id pgs_args_print_help_specific_id
        #define args_print_help_specific_name pgs_args_print_help_specific_name

    #endif // PGS_ARGS_STRIP_PREFIX

#endif // PGS_ARGS_STRIP_PREFIX_GUARD_

/*
    Revision History:

        0.3.1 (2026-05-11) Fix c++ compatibility warnings
                            - -Wswitch fix
                            - explicit conversion

        0.3.0 (2026-05-11) Built In Validators and parsing helpers, improve help string
                            - validator for valid input
                                - int, uint, pos int, generic numbers

        0.2.0 (2026-04-12) Subcommand support
                            - configurable names for multiple independant instances

        0.1.0 (2025-12-22) Initial Release
*/


/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025-2026 Paul Geisthardt
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
