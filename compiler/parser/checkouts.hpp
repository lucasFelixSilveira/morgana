#pragma once

#include "statements.hpp"
#include "symbols.hpp"
#include <regex>
#include <string>
#include <variant>

enum STARTS_WITH { ST_TYPEMENT, ST_IDENTIFIER, ST_STATEMENT, ST_NUMBER, ST_STRING, ST_UNKOWN };

STARTS_WITH starts_expr(std::string token) {
    symbol sym = sfrom(token);

    if( std::regex_match(token, std::regex("[-]?[0-9]+")) ) return ST_NUMBER;
    if( std::regex_match(token, std::regex("[\"](.*)[\"]")) ) return ST_STRING;
    if(! std::holds_alternative<std::monostate>(sym) ) return ST_TYPEMENT;

    #define X(id, ...) if( token == id ) return ST_STATEMENT;
    MORGANA_PARSER_STATEMENTS_FIELDS
    #undef X

    return std::regex_match(token, std::regex("^[a-zA-Z_][a-zA-Z0-9_]*$")) ? ST_IDENTIFIER : ST_UNKOWN;
}
