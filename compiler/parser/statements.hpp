#pragma once

#include <cstdint>

const uint8_t maybe = 2;

#define MORGANA_PARSER_STATEMENTS_FIELDS \
    X("constant", true,  constant)       \
    X("alloc",    true,  alloc)          \
                                         \
    X("comptime", false, comptime)       \
    X("store",    false, store)          \
    X("puts",     false, puts)           \
    X("ret",      false, ret)
