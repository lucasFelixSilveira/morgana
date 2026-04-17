#pragma once

// #include "statements/store.hpp"
// #include "statements/ret.hpp"

#define MORGANA_PARSER_STATEMENTS_FIELDS \
    X("alloc", true, alloc) \
    X("store", false, store) \
    X("ret", false, ret)
