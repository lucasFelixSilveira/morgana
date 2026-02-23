#pragma once

#include "../../parser.hpp"
#include "../symbols.hpp"
#include "../checkout.hpp"
#include "bool.hpp"
#include "integer.hpp"
#include "strong_alias.hpp"
#include <variant>

using validation = std::tuple<bool, symbol>;

#define REDEF_DATA_TYPE(type, ctx) \
    if( std::holds_alternative<type>(alias) && contains<int>(accept, ctx) ) { \
        data = std::get<type>(alias); \
        goto ignore_next; \
    }

validation check_valid(const std::string& type, const std::string& value, int ctx) {
    auto opt_data = symbol_table.lookup(type);
    if(! opt_data ) return { false, symbol{} };

    symbol data = *opt_data;
    bool is_alias = false;
    auto strong_alias = morgana_strong_alias(std::monostate(), 0);

    if( std::holds_alternative<morgana_strong_alias>(data) ) {
        auto [alias, accept] = std::get<morgana_strong_alias>(data);
        REDEF_DATA_TYPE(morgana_integer, ctx)
        REDEF_DATA_TYPE(morgana_bool, ctx)
    }

    /* Used to ignore the others "if" verifications
     * if one was successful to get more performance */
    ignore_next: {};

    if( std::holds_alternative<morgana_integer>(data) ) {
        auto integer = std::get<morgana_integer>(data);
        return { integer.check(value), data };
    }

    if( std::holds_alternative<morgana_bool>(data) ) {
        auto boolean = std::get<morgana_bool>(data);
        return { boolean.check(value), data };
    }

    return { false, data };
}
