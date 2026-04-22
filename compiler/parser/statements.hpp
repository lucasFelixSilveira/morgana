#pragma once

#include <cstdint>

const uint8_t whatever = 67;
#define MORGANA_PARSER_STATEMENTS_FIELDS \
    X("constant", true,     constant)       \
    X("alloc",    true,     alloc)          \
    X("load",     true,     load)           \
                                            \
    X("comptime", false,    comptime)       \
    X("store",    false,    store)          \
    X("puts",     false,    puts)           \
    X("ret",      false,    ret)            \
                                            \
    X("call",     whatever, call)
