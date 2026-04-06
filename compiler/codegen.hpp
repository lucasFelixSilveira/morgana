#pragma once

#include "compiler_outputs.hpp"
#include "extensors/runtime.hpp"
#include <iostream>
#include <memory>
#include <stack>
#include <stddef.h>
#include "libs/linux/include/runa.hpp"
#include "parser/declaration.hpp"
#include "parser/storage.hpp"
#include "parser/symbols.hpp"
#include "parser/types/integer.hpp"
#include "runa.hpp"
#include "params.hpp"
#include "parser.hpp"
#include "parser/checkout.hpp"
#include "parser/function.hpp"
#include <string>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

int function_id = 0;

using iteration = std::tuple<int, ParseResults>;
using iterator = std::stack<iteration>;

std::shared_ptr<iteration> current;
iterator branches;

struct Preprocessor {
    using value = std::variant<std::monostate, size_t, std::string>;
    std::stack<std::vector<std::pair<std::string, value>>> stack;

    Preprocessor() = default;

    void push() { stack.push({}); }
    void pop() { stack.pop(); }

    void add(std::string &identifier, value val) {
        auto top = stack.top();
        top.push_back({ identifier, val });
        stack.pop();
        stack.push(top);
    }

    value lookup(std::string &identifier) {
        auto top = stack.top();
        for( auto &[id, val] : top ) {
            if(id == identifier) {
                return val;
            }
        }
        return value(std::monostate());
    }
};

function fn;

struct Symbols;
struct SPOS {
    int stack_position;
    struct { int bytes; int matirx; bool ptr; } data;
    static SPOS from(Symbols&, symbol);
};

struct Symbols {
    int stack_pos;
    Preprocessor preprocessor;
    std::stack<std::vector<std::pair<std::string, SPOS>>> stack;

    Symbols() : stack_pos(0), stack() {}

    void add(std::string &identifier, SPOS sym) {
        auto top = stack.top();
        stack.pop();
        top.push_back({ identifier, sym });
        stack_pos += sym.data.bytes;
        stack.push(top);
    }

    SPOS lookup(std::string &identifier) {
        auto top = stack.top();
        for( auto &[id, sp] : top ) {
            if(id == identifier) {
                return sp;
            }
        }
        CompilerOutputs::Fatal("lookup failed for " + identifier);

        return {};
    }
};

SPOS SPOS::from(Symbols& sym, symbol s) {
    if( std::holds_alternative<morgana_integer>(s) ) {
        auto value = std::get<morgana_integer>(s);
        return SPOS{ sym.stack_pos + (value.bits / 8), { (value.bits / 8), value.matrixPos(), false } };
    };
    return {};
}

void morgana_push_ctx(function data) {
    fn = data;
    ParseResults results = parse(data.body);
    branches.push(*current);
    current = std::make_shared<iteration>(0, results);
}

std::string output;
std::string ident;

void tabs(Runa *runa) {
    RunaValueFFI val = runa_peek_arg(runa, 0);
    size_t count = val.data.integer;
    ident = std::string(count, '\t');
    runa_value_free(val);
}

void write(Runa *runa) {
    RunaValueFFI val = runa_peek_arg(runa, 0);
    char *str = runa_value_to_string(runa, val);
    output += std::string(str);
    runa_optional(RUNA_FREE_STRING_BY_VALUE, runa_str_free, str, val);
    runa_value_free(val);
}

void writeln(Runa *runa) {
    RunaValueFFI val = runa_peek_arg(runa, 0);
    char *str = runa_value_to_string(runa, val);
    output += ident + std::string(str) + "\n";
    runa_optional(RUNA_FREE_STRING_BY_VALUE, runa_str_free, str, val);
    runa_value_free(val);
}


void table(Symbols& symbols, Runa *runa, ParseResult& ast);
void epilogue(Symbols& symbols, Runa *runa);

std::string codegen(CompilerParams& params, ParseResults ast) {
    std::string path = Runtime::get_executable_path("morgana");
    auto [extensorExists, extensorPath] = Runtime::check_extensors(path, params.target);

    if(! extensorExists ) {
        CompilerOutputs::Fatal("No extensor found for target: " + params.target);
        return "";
    }

    CompilerOutputs::Info("Extensor found: " + extensorPath);
    CompilerOutputs::Info("Generating code via extensor");

    current = std::make_shared<iteration>(0, ast);

    Symbols symbols;

    Runa *runa = runa_start();
    runa_needs(runa, write, writeln, tabs);
    runa_loadfile(runa, (char*) extensorPath.c_str());

    while(true) {
        reset_fields();

        auto& [index, data] = *current;

        if( index < data.size() ) {
            index++;
            table(symbols, runa, data.at(index - 1));
            runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
            continue;
        } else {
            if( branches.empty() ) break;
            else { epilogue(symbols, runa); }
            current = std::make_shared<iteration>(branches.top());
            branches.pop();
        }
    }

    runa_free(runa);
    return output;
}

void table(Symbols& symbols, Runa *runa, ParseResult& node) {
    runa_push_field(runa, (char*) "kind", RunaValueFFI {
        runa_integer, RunaValueData { .integer = (size_t) first(node) }
    });

    add_field();
    switch(first(node)) {
        case ParseResultKind::Function: {
            symbols.stack.push({});
            symbols.preprocessor.push();

            auto data = std::get<function>(second(node));
            add_fields(2);

            runa_push_field(runa, (char*) "name",
            RunaValueFFI { runa_string, RunaValueData { .string = data.name.c_str() }});

            runa_push_field(runa, (char*) "id",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) function_id }});

            morgana_push_ctx(data);
        } break;

        case ParseResultKind::Label:
        case ParseResultKind::Branch: {
            auto data = std::get<std::string>(second(node));
            add_fields(2);

            runa_push_field(runa, (char*) "fn",
            RunaValueFFI { runa_string, RunaValueData { .string = fn.name.c_str() }});

            runa_push_field(runa, (char*) "name",
            RunaValueFFI { runa_string, RunaValueData { .string = data.c_str() }});
        } break;

        case ParseResultKind::Alloc: {
            auto [identifier, type] = std::get<declaration<symbol>>(second(node));
            SPOS to = SPOS::from(symbols, type);
            symbols.add(identifier, to);
        } break;

        case ParseResultKind::Store: {
            auto store = std::get<storage>(second(node));
            add_fields(6);

            SPOS lhs_value, rhs_value = symbols.lookup(store.identifier);

            runa_push_field(runa, (char*) "dest",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) rhs_value.stack_position }});

            size_t val = 0;
            size_t stack = 0;
            size_t lhs = 0, rhs = 0;
            size_t is_literal = is_number(store.value) || ((! is_number(store.value)) && std::holds_alternative<size_t>(symbols.preprocessor.lookup(store.value)));

            if( is_literal ) {
                if( is_number(store.value) ) val = (size_t) std::stoi(store.value);
                else val = std::get<size_t>(symbols.preprocessor.lookup(store.value));
            } else {
                lhs_value =  symbols.lookup(store.value);
                stack = (size_t) lhs_value.stack_position;
            }

            rhs = rhs_value.data.bytes;
            if( is_literal ) lhs = rhs;
            else lhs = lhs_value.data.bytes;

            runa_push_field(runa, (char*) "lhs",
            RunaValueFFI { runa_integer, RunaValueData { .integer = lhs }});

            runa_push_field(runa, (char*) "rhs",
            RunaValueFFI { runa_integer, RunaValueData { .integer = rhs }});

            runa_push_field(runa, (char*) "value",
            RunaValueFFI { runa_integer, RunaValueData { .integer = val }});

            runa_push_field(runa, (char*) "stack",
            RunaValueFFI { runa_integer, RunaValueData { .integer = stack }});

            runa_push_field(runa, (char*) "is_literal",
            RunaValueFFI { runa_integer, RunaValueData { .integer = is_literal }});
        } break;

        case ParseResultKind::Load: {
            auto [identifier, load] = std::get<declaration<std::string>>(second(node));
            symbols.add(identifier, symbols.lookup(load));
        } break;

        case ParseResultKind::Operation: {
            auto [identifier, operation] = std::get<declaration<morgana_operation>>(second(node));

            if( is_number(operation.lhs) && is_number(operation.rhs) ) {
                size_t val;
                if( operation.instruction == "add" ) val = std::stoi(operation.lhs) + std::stoi(operation.rhs);
                if( operation.instruction == "sub" ) val = std::stoi(operation.lhs) - std::stoi(operation.rhs);
                if( operation.instruction == "mul" ) val = std::stoi(operation.lhs) * std::stoi(operation.rhs);
                if( operation.instruction == "div" ) val = std::stoi(operation.lhs) / std::stoi(operation.rhs);
                symbols.preprocessor.add(identifier, val);
                reset_fields();
                break;
            }

            Preprocessor::value lhs;
            Preprocessor::value rhs;

            if( is_number(operation.lhs) ) lhs = Preprocessor::value((size_t) std::stoi(operation.lhs));
            else lhs = symbols.preprocessor.lookup(operation.lhs);

            if( is_number(operation.rhs) ) rhs = Preprocessor::value((size_t) std::stoi(operation.rhs));
            else rhs = symbols.preprocessor.lookup(operation.rhs);

            size_t val;

            if(! ( std::holds_alternative<std::monostate>(lhs) || std::holds_alternative<std::monostate>(rhs) ) ) {
                if( operation.instruction == "add" ) val = std::get<size_t>(lhs) + std::get<size_t>(rhs);
                if( operation.instruction == "sub" ) val = std::get<size_t>(lhs) - std::get<size_t>(rhs);
                if( operation.instruction == "mul" ) val = std::get<size_t>(lhs) * std::get<size_t>(rhs);
                if( operation.instruction == "div" ) val = std::get<size_t>(lhs) / std::get<size_t>(rhs);
                symbols.preprocessor.add(identifier, val);
                reset_fields();
                break;
            }

        }
    }
}

void epilogue(Symbols& symbols, Runa *runa) {
    reset_fields();

    runa_push_field(runa, (char*) "kind",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) Epilogue }});

    runa_push_field(runa, (char*) "name",
    RunaValueFFI { runa_string, RunaValueData { .string = fn.name.c_str() } });

    runa_push_field(runa, (char*) "id",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) function_id++ }});

    runa_push_field(runa, (char*) "stack",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) symbols.stack_pos }});

    symbols.stack.pop();
    symbols.preprocessor.pop();
    symbols.stack_pos = 0;
    add_fields(4);
    runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
}




























// struct ASTIterator {
//     ParseResults ast;
//     size_t current_index;
// };

// #define JSON_ENCODER(ss, vec, constructor)                                    \
//     {                                                                         \
//         auto build = [&](auto& iter) -> std::string constructor;              \
//         ss << "{ \"data\": [";                                                \
//         bool first = true;                                                    \
//         for( auto iter : vec ) {                                              \
//             ss << ((first) ? "" : ", ") << build(iter) << "";                 \
//             if( first ) first = !first;                                       \
//         };                                                                    \
//         ss << "] }";                                                          \
//     }

// #define CALL_FIELD_FOR_SYMBOLS(ret, symbol, field)                            \
//     [&]() -> ret {                                                            \
//         if( std::holds_alternative<morgana_integer>(symbol) )                 \
//         /* -> */ return std::get<morgana_integer>(symbol).field();            \
//         if( std::holds_alternative<morgana_tuple>(symbol) )                   \
//         /* -> */ return std::get<morgana_tuple>(symbol).field();              \
//         return ret();                                                         \
//     }()



// std::stack<std::shared_ptr<ASTIterator>> iterators;
// std::shared_ptr<ASTIterator> current_iterator = nullptr;

// static void morgana_push_ctx(std::vector<std::string> ctx) {
//     if( current_iterator ) {
//         auto saved_iterator = std::make_shared<ASTIterator>();
//         saved_iterator->ast = current_iterator->ast;
//         saved_iterator->current_index = current_iterator->current_index;
//         iterators.push(saved_iterator);
//     }

//     auto new_iterator = std::make_shared<ASTIterator>();
//     new_iterator->ast = parse(ctx);
//     new_iterator->current_index = 0;
//     current_iterator = new_iterator;
// }

// static bool file_exists(const std::string& path) {
//     return access(path.c_str(), R_OK) == 0;
// }

// static std::string find_lua_module(const std::string& dir, const std::string& modname) {
//     std::string modpath = modname;
//     std::replace(modpath.begin(), modpath.end(), '.', '/');

//     std::vector<std::string> candidates = {
//         dir + modname + ".lua",
//         dir + modname + "/init.lua"
//     };

//     for( const auto& candidate : candidates )
//         if( file_exists(candidate) ) return candidate;

//     return "";
// }

// static void push_ast_node_to_lua(lua_State* L, ParseResult& node) {
//     lua_newtable(L);

//     lua_pushinteger(L, first(node));
//     lua_setfield(L, -2, "kind");

//     switch(first(node)) {
//         case ParseResultKind::Function: {
//             auto data = std::get<function>(second(node));
//             std::stringstream ss;

//             // append the current iterator on the stack
//             // and then put the function iterator on the
//             // current_iterator global variable
//             morgana_push_ctx(data.body);

//             // store the function name on the table
//             // for lua know all the IDs of the function
//             lua_pushstring(L, data.name.c_str());
//             lua_setfield(L, -2, "name");

//             // store the function parameters type
//             // using JSON encoder
//             JSON_ENCODER(ss, data.argst, {
//                 if( std::holds_alternative<morgana_strong_alias>(iter) ) {
//                     auto data = std::get<0>(std::get<morgana_strong_alias>(iter));
//                     if( std::holds_alternative<morgana_integer>(data) ) {
//                         auto x = std::get<morgana_integer>(data);
//                         iter = symbol(x);
//                     }
//                 }

//                 if( std::holds_alternative<morgana_integer>(iter) ) return std::get<morgana_integer>(iter).json();
//                 return std::string();
//             });
//             lua_pushstring(L, ss.str().c_str());
//             lua_setfield(L, -2, "params");

//             ss.str("");
//             ss.clear();
//         } break;

//         case ParseResultKind::Loop: {
//             auto data = std::get<loop>(second(node));

//             // append the current iterator on the stack
//             // and then put the loop iterator on the
//             // current_iterator global variable
//             morgana_push_ctx(data.body);
//         } break;

//         case ParseResultKind::BranchEqualZero:
//         case ParseResultKind::BranchNotEqualZero: {
//             auto data = std::get<simplebranch>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             lua_pushstring(L, second(data).c_str());
//             lua_setfield(L, -2, "label");
//         } break;

//         case ParseResultKind::BranchGrant:
//         case ParseResultKind::BranchLess:
//         case ParseResultKind::BranchGrantEqual:
//         case ParseResultKind::BranchLessEqual: {
//             auto data = std::get<branchmeasure>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "first");

//             lua_pushstring(L, second(data).c_str());
//             lua_setfield(L, -2, "second");

//             lua_pushstring(L, third(data).c_str());
//             lua_setfield(L, -2, "label");
//         } break;

//         case ParseResultKind::Branch: {
//             auto data = std::get<std::string>(second(node));

//             lua_pushstring(L, data.c_str());
//             lua_setfield(L, -2, "label");
//         } break;

//         case ParseResultKind::Label: {
//             auto data = std::get<std::string>(second(node));

//             lua_pushstring(L, data.c_str());
//             lua_setfield(L, -2, "identifier");
//         } break;

//         case ParseResultKind::AddInPtr: {
//             auto data = std::get<declaration<addinptr>>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             addinptr into = second(data);
//             lua_pushstring(L, first(into).c_str());
//             lua_setfield(L, -2, "address");

//             lua_pushinteger(L, second(into));
//             lua_setfield(L, -2, "offset");
//         } break;

//         case ParseResultKind::Wait:
//         case ParseResultKind::WaitMS: {
//             auto data = std::get<int>(second(node));

//             // calculate the delay in milliseconds
//             // considering the base unit is seconds
//             // for the `wait` keyword.
//             const int second = 1000;
//             int base = ParseResultKind::Wait == first(node);
//             int mul = base * second + !base;
//             lua_pushinteger(L, data * mul);
//             lua_setfield(L, -2, "ms");
//         } break;

//         case ParseResultKind::Turn: {
//             auto data = std::get<turn>(second(node));

//             lua_pushinteger(L, data.pin);
//             lua_setfield(L, -2, "pin");

//             lua_pushboolean(L, data.toggle);
//             lua_setfield(L, -2, "toggle");
//         } break;

//         case ParseResultKind::Read: {
//             auto data = std::get<declaration<mcu_read>>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             lua_pushinteger(L, first(second(data)));
//             lua_setfield(L, -2, "pin");

//             lua_pushboolean(L, second(second(data)));
//             lua_setfield(L, -2, "digital");
//         } break;

//         case ParseResultKind::Alloc: {
//             auto data = std::get<declaration<symbol>>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             auto type = second(data);
//             auto json = CALL_FIELD_FOR_SYMBOLS(std::string, type, json);
//             lua_pushstring(L, json.c_str());
//             lua_setfield(L, -2, "type");
//         } break;

//         case ParseResultKind::Load: {
//             auto data = std::get<declaration<std::string>>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             lua_pushstring(L, second(data).c_str());
//             lua_setfield(L, -2, "source");
//         } break;

//         case ParseResultKind::Store: {
//             auto data = std::get<storage>(second(node));

//             lua_pushstring(L, data.identifier.c_str());
//             lua_setfield(L, -2, "identifier");

//             lua_pushstring(L, data.value.c_str());
//             lua_setfield(L, -2, "value");
//         } break;

//         case ParseResultKind::Operation: {
//             auto data = std::get<declaration<morgana_operation>>(second(node));

//             lua_pushstring(L, first(data).c_str());
//             lua_setfield(L, -2, "identifier");

//             auto info = second(data);

//             lua_pushstring(L, info.instruction.c_str());
//             lua_setfield(L, -2, "instruction");

//             lua_pushstring(L, info.lhs.c_str());
//             lua_setfield(L, -2, "lhs");

//             lua_pushstring(L, info.rhs.c_str());
//             lua_setfield(L, -2, "rhs");
//         } break;

//         default: break;
//     }
// }

// static int morgana_next(lua_State* L) {
//     if(! current_iterator || current_iterator->ast.empty() ) {
//         lua_pushnil(L);
//         lua_pushstring(L, "No AST available or empty AST");

//         if( iterators.size() > 0 ) {
//             auto restored = iterators.top();
//             iterators.pop();
//             current_iterator = restored;
//         }

//         return 2;
//     }

//     if( current_iterator->current_index >= current_iterator->ast.size() ) {
//         if (iterators.empty()) {
//             lua_pushnil(L);
//             lua_pushstring(L, "End of AST");
//             return 2;
//         }

//         auto restored = iterators.top();
//         iterators.pop();
//         current_iterator = restored;

//         if(! current_iterator || current_iterator->ast.empty() ) {
//             lua_pushnil(L);
//             lua_pushstring(L, "No AST after restoration");
//             return 2;
//         }

//         if( current_iterator->current_index >= current_iterator->ast.size() ) {
//             lua_pushnil(L);
//             lua_pushstring(L, "Restored iterator also finished");
//             return 2;
//         }
//     }

//     auto node = current_iterator->ast.at(current_iterator->current_index);
//     current_iterator->current_index++;

//     push_ast_node_to_lua(L, node);
//     return 1;
// }

// static int morgana_reset(lua_State* L) {
//     lua_pushboolean(L, true);
//     return 1;
// }

// static int morgana_require(lua_State* L) {
//     const char* modname = luaL_checkstring(L, 1);

//     lua_getfield(L, LUA_REGISTRYINDEX, "morgana_extensor_dir");
//     const char* dir_cstr = lua_tostring(L, -1);
//     std::string dir = dir_cstr ? dir_cstr : "./";
//     lua_pop(L, 1);

//     std::string module_file = find_lua_module(dir, modname);

//     if( module_file.empty() ) {
//         lua_pushnil(L);
//         lua_pushfstring(L, "module '%s' not found in '%s'", modname, dir.c_str());
//         return 2;
//     }

//     if( luaL_loadfile(L, module_file.c_str()) != LUA_OK ) return lua_error(L);

//     lua_call(L, 0, 1);
//     return 1;
// }
