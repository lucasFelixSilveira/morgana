#pragma once

#include "types/integer.hpp"
#include "types/ptr.hpp"
#include "types/void.hpp"
#include <variant>

using symbol = std::variant<
    std::monostate,
    /* Morgana types who are reachable by the user when
     * generating/writing a Morgana program.
     *
     * There is a radical syntax definition for each of this types.
     * And, for sure, also there is a check function who verifies
     * if a string (received from lexer) is a valid symbol of this type.
     */
    morgana_integer,
    morgana_void,
    morgana_ptr
>;


#define MORGANA_SYMBOLS_FIELDS \
    X(morgana_i8,   "i8",   morgana_integer(false, 8 )) X(morgana_u8,  "u8",  morgana_integer(true, 8) ) \
    X(morgana_i16,  "i16",  morgana_integer(false, 16)) X(morgana_u16, "u16", morgana_integer(true, 16)) \
    X(morgana_i32,  "i32",  morgana_integer(false, 32)) X(morgana_u32, "u32", morgana_integer(true, 32)) \
    X(morgana_i64,  "i64",  morgana_integer(false, 64)) X(morgana_u64, "u64", morgana_integer(true, 64)) \
    X(morgana_void, "void", morgana_void()) \
    X(morgana_ptr,  "ptr",  morgana_ptr())

symbol sfrom(std::string identifier) {
    #define X(name, str, type) if( identifier == str ) return type;
    MORGANA_SYMBOLS_FIELDS
    #undef X
    return std::monostate();
};
