#ifndef RUNA_H
#define RUNA_H
#include <stddef.h>
#include <stdbool.h>

#define RUNA_TAG_FIELDS \
    X(runa_string,    "string"), \
    X(runa_integer,   "integer"), \
    X(runa_float,     "float"), \
    X(runa_boolean,   "boolean"), \
    X(runa_table,     "table"), \
    X(__internal_ptr, "pointer"), \
    X(runa_nil,       "nil")

#define X(tag, name) tag
typedef enum {
    RUNA_TAG_FIELDS
} RunaValueTag;
#undef X

#define make_string(str) (RunaValueFFI){ .tag = runa_string, .data.string = str }
#define make_integer(int) (RunaValueFFI){ .tag = runa_integer, .data.integer = int }
#define make_float(float) (RunaValueFFI){ .tag = runa_float, .data._float = float }
#define make_boolean(bool) (RunaValueFFI){ .tag = runa_boolean, .data.boolean = bool }
#define make_nil() (RunaValueFFI){ .tag = runa_nil, .data = { 0 } }

#define as_string(val) ((RunaValueFFI)val).data.string
#define as_integer(val) ((RunaValueFFI)val).data.integer
#define as_float(val) ((RunaValueFFI)val).data._float
#define as_boolean(val) ((RunaValueFFI)val).data.boolean

typedef union {
    const char* string;
    size_t integer;
    double _float;
    bool boolean;
} RunaValueData;

typedef struct {
    RunaValueTag tag;
    RunaValueData data;
} RunaValueFFI;

typedef struct Runa Runa;
typedef void (*runa_callback)(Runa *);

enum {
    RUNA_FREE_STRING_BY_VALUE
};

#define runa_optional(funcid, cb, x, y)               \
switch (funcid) {                                     \
    case RUNA_FREE_STRING_BY_VALUE: {                 \
        if( ((RunaValueFFI) y).tag != runa_string ) { \
            cb((char*)x);                             \
        }                                             \
    } break;                                          \
}

/* push instruictions */
void runa_push_field(Runa *runa, char *key, RunaValueFFI value);
void runa_push_table(Runa *runa, char *identifier, unsigned int amount);
void runa_push_function(Runa *runa, char *name, runa_callback callback, int argc);
void runa_push_result(Runa *runa, RunaValueFFI value);

/* function utils */

// This peeks at an argument value by their position in argument stack.
RunaValueFFI runa_peek_arg(Runa *runa, int index);
RunaValueFFI runa_spawn_function(Runa *runa, char *name, runa_callback cap);

/* values */

// This clean up a value, freeing any associated memory.
void runa_value_free(RunaValueFFI value);

// This converts a value to a string representation.
char* runa_value_to_string(Runa *runa, RunaValueFFI value);

// This frees a string returned by `runa_value_to_string`.
void runa_str_free(char *str);

/* runa internals */

// This loads a file into the runa statement.
void runa_loadfile(Runa *runa, char *filename);

// This starts the runa statement.
Runa *runa_start();

// This frees the runa statement.
void runa_free(Runa *runa);

#endif
