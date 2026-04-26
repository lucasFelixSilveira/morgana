#pragma once

#include "blocks.hpp"
#include "call.hpp"
#include "checkouts.hpp"
#include "function.hpp"
#include "puts.hpp"
#include "ret.hpp"
#include "statements.hpp"
#include "storage.hpp"
#include "storage.hpp"
#include "symbols.hpp"
#include "../compiler_outputs.hpp"
#include "types/integer.hpp"
#include "types/ptr.hpp"
#include <cstdint>
#include <iostream>
#include <regex>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#define REGEX true
#define TO_PARSE false

#define MORGANA_PARSER_NODE_FIELDS \
    X(PUTS,      0)  \
    X(STRINGS,   1)  \
    X(DATA,      2)  \
    X(TEXT,      3)  \
    X(COMPTIMNE, 4)  \
    X(FUNCTION, 100) \
    X(EPILOGUE, 101) \
    X(RET,      102) \
    X(CALL,     103) \
    X(ALLOC,    200) \
    X(STORE,    201) \
    X(LOAD,     202) \

#define X(name, id) name = id,
enum parse_kind : int32_t { MORGANA_PARSER_NODE_FIELDS };
#undef X

using strings = std::unordered_map<std::string, std::string>;
std::stack<strings> strings_stack;

template<typename T>
using declaration = std::tuple<std::string, T>;

using ParseResult = std::tuple<parse_kind, std::variant<
    std::monostate,
    std::string,
    strings,
    function,
    storage,
    puts_t,
    ret_t,
    call_t,

    /* declarations */
    declaration<symbol>,
    declaration<call_t>,
    declaration<std::string>
>>;

template<typename T>
auto first(T x) { return std::get<0>(x); }

template<typename T>
auto second(T x) { return std::get<1>(x); }

using ParseResults = std::vector<ParseResult>;

#define MORGANA_STATEMENT_FUNCTION_ARGUMENTS int& i, std::vector<std::string>& tokens, std::string identifier, ParseResults& result, bool is_declaration

#define X(id, def, fn) void fn(MORGANA_STATEMENT_FUNCTION_ARGUMENTS);
MORGANA_PARSER_STATEMENTS_FIELDS
#undef X

ParseResults parse(std::vector<std::string> tokens) {
    std::cout << "parser\n";
    ParseResults result;
    block::init();
    block::push_generic();
    strings_stack.push({});

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
                #define X(id, def, fn) if( operation == id && def != 0 ) { fn(i, tokens, first, result, true); continue; }
                MORGANA_PARSER_STATEMENTS_FIELDS
                #undef X
            } continue;

            case ST_STATEMENT: {
                /* Auto caller from all statements who DO NOT need to be used
                 * after a definition. */
                #define X(id, def, fn) if( first == id && (def == 0 || def == whatever) ) { fn(i, tokens, first, result, false); continue; }
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

void call(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&](int j) { (j == 0) ? CompilerOutputs::Fatal("After a `call` you need to place a valid function")
                                     : CompilerOutputs::Fatal("You need to place a valid value in a call argument"); };

    if( i + 1 >= tokens.size() ) err(0);

    std::string tk = tokens.at(++i);
    std::regex func = std::regex("(.*)\\((.*)");
    std::smatch matches;
    if(! std::regex_search(tk, matches, func) ) err(0);

    std::string function = matches[1];
    std::string arguments = matches[2];
    std::vector<std::string> args({});

    while(1) {
        char ch = arguments[arguments.length()-1];
        if( ch == ')' || ch == ',' ) args.push_back(arguments.substr(0, arguments.length()-1));
        if( ch == ')' ) break;

        arguments = tokens.at(++i);
    }

    if( is_declaration ) result.push_back({ parse_kind::CALL, declaration<call_t> { identifier, call_t(function, args) } });
    else result.push_back({ parse_kind::CALL, call_t(function, args) });
}

void load(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&]() { CompilerOutputs::Fatal("After a `load` you need to place a valid allocation"); };
    if( i + 1 >= tokens.size() ) err();

    std::string alloc = tokens.at(++i);
    STARTS_WITH check = starts_expr(alloc);
    if( check != STARTS_WITH::ST_IDENTIFIER ) err();

    if(! block::lookup(block::allocations, alloc) ) block::error(block::allocations, alloc);

    result.push_back({ parse_kind::LOAD, declaration<std::string> { identifier, alloc } });
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

    switch(check) {
        case STARTS_WITH::ST_NUMBER: {} break;
        case STARTS_WITH::ST_IDENTIFIER: {
            if( block::lookup(block::constants, value) ) {
                auto [_, content] = block::peek(block::constants, value);
                if( content.empty() ) {
                    auto& top = strings_stack.top();
                    if( top.find(value) == top.end() ) err(1);
                }
                break;
            }
        } break;
        default: err(1);
    }

    auto [_, symbol] = block::peek(block::allocations, expr);
    if( std::holds_alternative<morgana_integer>(symbol) ) {
        auto int_t = std::get<morgana_integer>(symbol);
        if(! int_t.check(value) ) err(1);
    }

    result.push_back({ parse_kind::STORE, storage(expr, value) });
}

/* Return statement implementation - [DO NOT NEED DECLARATION]
 * - Return was used to return a value from a function. */
void ret(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    if( i + 1 >= tokens.size() ) {
        result.push_back({ parse_kind::RET, ret_t(std::monostate()) });
        return;
    }

    ret_t v = std::monostate();
    std::string value = tokens.at(++i);
    STARTS_WITH check = starts_expr(value);
    if( check == STARTS_WITH::ST_IDENTIFIER || check == STARTS_WITH::ST_NUMBER ) v = value;

    result.push_back({ parse_kind::RET, v });
}

void puts(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&]() { CompilerOutputs::Fatal("After a `puts` statement you need to place a valid constant of string"); };
    if( i + 1 >= tokens.size() ) err();
    std::string value = tokens.at(++i);

    STARTS_WITH check = starts_expr(value);
    if( check != STARTS_WITH::ST_IDENTIFIER ) err();

    auto& top = strings_stack.top();
    if( top.find(value) == top.end() ) err();

    result.push_back({ parse_kind::PUTS, puts_t(value, top.at(value).size()) });
}

void constant(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&]() { CompilerOutputs::Fatal("After a `constant` statement you need to place a valid value or constant statement"); };

    if( i + 1 >= tokens.size() ) err();
    std::string value = tokens.at(++i);

    STARTS_WITH check = starts_expr(value);
    switch(check) {
        case STARTS_WITH::ST_NUMBER: {
            block::push_back(block::constants, block::constant_t { identifier, value });
        } break;
        case STARTS_WITH::ST_STRING: {
            auto top = strings_stack.top();
            top.insert({identifier, value});
            strings_stack.pop();
            strings_stack.push(top);
            block::push_back(block::constants, block::constant_t { identifier, std::string() });
        } break;
        default: err();
    }
}

void comptime(MORGANA_STATEMENT_FUNCTION_ARGUMENTS) {
    auto err = [&]() { CompilerOutputs::Fatal("After a `comptime` statement you need to place a valid value or comptime statement"); };

    if( i + 1 >= tokens.size() ) err();
    std::string value = tokens.at(++i);
    result.push_back({ parse_kind::COMPTIMNE, value });
    return;
}
