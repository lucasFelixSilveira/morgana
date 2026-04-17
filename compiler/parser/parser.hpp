#pragma once

#include "blocks.hpp"
#include "checkouts.hpp"
#include "function.hpp"
#include "statements.hpp"
#include "storage.hpp"
#include "storage.hpp"
#include "symbols.hpp"
#include "../compiler_outputs.hpp"
#include <cstdint>
#include <regex>
#include <stack>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#define REGEX true
#define TO_PARSE false

#define MORGANA_PARSER_NODE_FIELDS \
    X(FUNCTION, 100) \
    X(EPILOGUE, 101) \
    X(ALLOC,    200) \
    X(STORE,    201)

#define X(name, id) name = id,
enum parse_kind : int32_t { MORGANA_PARSER_NODE_FIELDS };
#undef X

template<typename T>
using declaration = std::tuple<std::string, T>;

using ParseResult = std::tuple<parse_kind, std::variant<
    std::monostate,
    function,
    storage,

    /* declarations */
    declaration<symbol>
>>;

template<typename T>
auto first(T x) { return std::get<0>(x); }

template<typename T>
auto second(T x) { return std::get<1>(x); }

using ParseResults = std::vector<ParseResult>;

#define MORGANA_STATEMENT_FUNCTION_ARGUMENTS int& i, std::vector<std::string>& tokens, std::string identifier, ParseResults& result

void alloc(MORGANA_STATEMENT_FUNCTION_ARGUMENTS);
void store(MORGANA_STATEMENT_FUNCTION_ARGUMENTS);

ParseResults parse(std::vector<std::string> tokens) {
    ParseResults result;
    block::init();
    block::push_generic();

    for( int i = 0; i < tokens.size(); i++ ) {
        const std::string& first = tokens.at(i);
        STARTS_WITH checkout = starts_expr(first);

        switch(checkout) {
            case ST_TYPEMENT: {
                /* Getting al the argument types */
                std::string values = tokens.at(++i);

                std::string x;
                std::string suffix = ")";
                while(std::equal(suffix.rbegin(), suffix.rend(), (x = tokens.at(++i)).rbegin())) values += x;

                /* Checking the function definition and splitting the
                 * name and arguments. */
                std::smatch match;
                bool is_valid = std::regex_match(values, match, std::regex("(.*)\\((.*)\\)"));
                if(! is_valid ) CompilerOutputs::Fatal("Invalid function definition: " + values);

                std::string name = match[1], args = match[2];

                /* Parsing the arguments into a vector of symbols
                 * and checking for valid symbols in argument. */
                std::vector<symbol> symbols;
                std::stringstream ss(args);
                std::string item;

                while(std::getline(ss, item, ',')) {
                    symbol sym = sfrom(item);
                    if(! std::holds_alternative<std::monostate>(sym) )
                    /* -> */ CompilerOutputs::Fatal("Invalid argument: " + item);
                    symbols.push_back(sym);
                }

                /* Checking for "noframe" flag before the body.
                 * If found, set no_frame to true and skip the "noframe" token.
                 *
                 * # What is "noframe"?
                 * Its an optional flag that can be used to indicate from
                 * the compiler if the function should be did run in a different
                 * stack frame than the old one.
                 *
                 * It is like a `move` in Rust, or a [&] in CPP
                 */
                bool no_frame = false;
                std::vector<std::string> body;
                std::string starts = tokens.at(i++);
                if( starts == "noframe" ) {
                    no_frame = true;
                    starts = tokens.at(i++);
                }

                /* Placing all the next tokens into the body vector until the
                 * closing brace. */
                if( starts == "{" ) {
                    while(tokens.at(i) != "}") body.push_back(tokens.at(i++));
                    i++;
                }

                i--;
                function fn(name, symbols, body, no_frame);
                result.push_back({ parse_kind::FUNCTION, fn });
            } continue;

            case ST_IDENTIFIER: {
                /* Checking for a '=' sign after the identifier.
                 * A valid definition must have a `=` sign after the identifier
                 * and a valid expression after it. */
                std::string sign = tokens.at(++i);
                if( sign != "=" ) CompilerOutputs::Fatal("After a identifier you need to place a '=' sign");

                /* Checking for a valid statement after the `=` sign.
                 * Some statements can't be used in definitions (e.g. `store`). */
                std::string operation = tokens.at(++i);
                STARTS_WITH check = starts_expr(operation);
                if( check != STARTS_WITH::ST_STATEMENT ) CompilerOutputs::Fatal("After a definition you need to place a valid statement");

                /* Auto caller from all the statements who need to be used
                 * after a definition. */
                #define X(id, def, fn) if( operation == id && def ) { fn(i, tokens, first, result); continue; }
                MORGANA_PARSER_STATEMENTS_FIELDS
                #undef X
            } continue;

            case ST_STATEMENT: {
                /* Auto caller from all statements who DO NOT need to be used
                 * after a definition. */
                #define X(id, def, fn) if( first == id && (! def ) ) { fn(i, tokens, first, result); continue; }
                MORGANA_PARSER_STATEMENTS_FIELDS
                #undef X
            } continue;
        }
    }

    block::pop_generic();
    return result;
}

/* Alloc statement implementation - [NEED DECLARATION]
 * - Alloc was used to allocate a memory space for a variable. */
void alloc(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&]() { CompilerOutputs::Fatal("After an `alloc` you need to place a valid type"); };
    if( i + 1 >= tokens.size() ) err();

    std::string type = tokens.at(++i);
    STARTS_WITH check = starts_expr(type);
    if( check != STARTS_WITH::ST_TYPEMENT ) err();

    declaration<symbol> declaration(identifier, sfrom(type));
    block::push_back(block::allocations, declaration);
    result.push_back({ parse_kind::ALLOC, declaration });
}

/* Store statement implementation - [DO NOT NEED DECLARATION]
 * - Store was used to store a value into a memory space. */
void store(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&](int j) { (j == 0) ? CompilerOutputs::Fatal("After a `store` you need to place a valid allocation")
                                     : CompilerOutputs::Fatal("In a `store` you need to place a valid value"); };
    if( i + 1 >= tokens.size() ) err(0);
    if( i + 2 >= tokens.size() ) err(1);

    std::string expr = tokens.at(++i);
    STARTS_WITH check = starts_expr(expr);
    if( check != STARTS_WITH::ST_IDENTIFIER ) err(0);

    if(! block::lookup(block::allocations, expr) ) block::error(block::allocations, expr);

    std::string value = tokens.at(++i);
    check = starts_expr(value);
    if( check != STARTS_WITH::ST_IDENTIFIER && check != STARTS_WITH::ST_NUMBER ) err(1);

    result.push_back({ parse_kind::STORE, storage(expr, value) });
}
