#pragma once

#include "compiler_outputs.hpp"
#include "extensors/runtime.hpp"
#include <cstddef>
#include <memory>
#include <regex>
#include <stack>
#include <stddef.h>
#include "parser/call.hpp"
#include "parser/ret.hpp"
#include "parser/statements.hpp"
#include "parser/types/integer.hpp"
#include "parser/types/ptr.hpp"
#include "runa.hpp"
#include "params.hpp"
#include "parser/parser.hpp"
#include "parser/function.hpp"
#include "parser/storage.hpp"
#include <string>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <utility>
#include <variant>
#include <vector>

extern "C" {
#include "libs/runa.h"
}

bool is_number(std::string &str)
{ return std::regex_match(str, std::regex("[-]?[0-9]+")); }

bool is_identifier(std::string &str)
{ return std::regex_match(str, std::regex("[A-Za-z_][A-Za-z0-9_]*")); }

int function_id = 0;

using iteration = std::tuple<int, ParseResults>;
using iterator = std::stack<iteration>;

std::shared_ptr<iteration> current;
iterator branches;

struct Preprocessor {
    using value = std::variant<std::monostate, size_t, std::string>;
    std::stack<std::vector<std::pair<std::string, value>>> stack;

    Preprocessor() = default;

    void push() {
        stack.push({});
    }

    void pop() {
        stack.pop();
    }

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
    struct { int bytes; bool ptr; } data;
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
        return SPOS{ sym.stack_pos + (value.bits / 8), { (value.bits / 8), false } };
    };
    if( std::holds_alternative<morgana_ptr>(s) ) {
        auto value = std::get<morgana_ptr>(s);
        char bytes = 8;
        return SPOS{ sym.stack_pos + bytes, { bytes, true } };
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
    #ifdef _WIN32
    output += ident + std::string(str) + "\r\n";
    #else
    output += ident + std::string(str) + "\n";
    #endif
    runa_optional(RUNA_FREE_STRING_BY_VALUE, runa_str_free, str, val);
    runa_value_free(val);
}

void table(Symbols& symbols, Runa *runa, ParseResult& ast);
void epilogue(Symbols& symbols, Runa *runa);
void pendent_strings_make(Runa *runa);

std::string codegen(CompilerParams& params, ParseResults ast) {
    std::string path = Runtime::get_executable_path("morgana");

    #ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
    #endif

    auto [extensorExists, extensorPath] = Runtime::check_extensors(path, params.target);

    if(! extensorExists ) {
        CompilerOutputs::Fatal("No extensor found for target: " + params.target);
        return "";
    }

    #ifdef _WIN32
    std::replace(extensorPath.begin(), extensorPath.end(), '/', '\\');
    #endif

    CompilerOutputs::Info("Extensor found: " + extensorPath);
    CompilerOutputs::Info("Generating code via extensor");

    current = std::make_shared<iteration>(0, ast);

    Symbols symbols;
    Runa *runa = runa_start();
    runa_needs(runa, write, writeln, tabs);

    std::vector<char> extensorPathCStr(extensorPath.begin(), extensorPath.end());
    extensorPathCStr.push_back('\0');

    runa_loadfile(runa, extensorPathCStr.data());

    int iteration_count = 0;
    while(true) {
        iteration_count++;
        reset_fields();
        auto& [index, data] = *current;

        if( index < data.size() ) {
            index++;
            table(symbols, runa, data.at(index - 1));
            runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
            continue;
        } else {
            if( branches.empty() ) {
                break;
            } else {
                epilogue(symbols, runa);
                pendent_strings_make(runa);
            }
            current = std::make_shared<iteration>(branches.top());
            branches.pop();
        }
    }

    runa_free(runa);

    return output;
}

void epilogue(Symbols& symbols, Runa *runa) {
    reset_fields();

    runa_push_field(runa, (char*) "kind",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) parse_kind::EPILOGUE }});

    runa_push_field(runa, (char*) "name",
    RunaValueFFI { runa_string, RunaValueData { .string = fn.name.c_str() } });

    runa_push_field(runa, (char*) "id",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) function_id }});

    runa_push_field(runa, (char*) "stack",
    RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) symbols.stack_pos }});

    symbols.stack.pop();
    symbols.preprocessor.pop();
    symbols.stack_pos = 0;
    add_fields(4);
    runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
}

void pendent_strings_make(Runa *runa) {
    reset_fields();

    add_field();
    runa_push_field(runa, (char*) "kind",
    RunaValueFFI { runa_integer, RunaValueData { .integer = parse_kind::DATA }});
    runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);

    reset_fields();
    add_fields(3);
    for( auto& [name, value] : strings_stack.top() ) {
        runa_push_field(runa, (char*) "kind",
        RunaValueFFI { runa_integer, RunaValueData { .integer = parse_kind::STRINGS }});

        runa_push_field(runa, (char*) "identifier",
        RunaValueFFI { runa_string, RunaValueData { .string = (".fn" + std::to_string(function_id) + "." + name).c_str() }});

        runa_push_field(runa, (char*) "value",
        RunaValueFFI { runa_string, RunaValueData { .string = value.c_str() }});

        runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
    }

    strings_stack.pop();
    function_id++;

    reset_fields();
}


void table(Symbols& symbols, Runa *runa, ParseResult& node) {
    runa_push_field(runa, (char*) "kind", RunaValueFFI {
        runa_integer, RunaValueData { .integer = (size_t) first(node) }
    });

    add_field();
    parse_kind kind = first(node);

    switch(kind) {
        case parse_kind::FUNCTION: {
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

        case parse_kind::RET: {
            auto data = std::get<ret_t>(second(node));
            add_fields(2);

            runa_push_field(runa, (char*) "is_empty",
            RunaValueFFI { runa_integer, RunaValueData { .integer = std::holds_alternative<std::monostate>(data) }});

            runa_push_field(runa, (char*) "id",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) function_id }});

            if( std::holds_alternative<std::monostate>(data) ) break;
            bool is_literal = false;
            std::string value;
            value = std::get<std::string>(data);

            if( std::holds_alternative<size_t>(symbols.preprocessor.lookup(value)) ) {
                is_literal = true;
                value = std::to_string(std::get<size_t>(symbols.preprocessor.lookup(value)));
            }
            else is_literal = is_number(value);

            add_fields(2);

            runa_push_field(runa, (char*) "is_literal",
            RunaValueFFI { runa_integer, RunaValueData { .integer = is_literal }});

            runa_push_field(runa, (char*) "value",
            RunaValueFFI { runa_string, RunaValueData {
                .string =
                    is_literal
                    ? value.c_str()
                    : std::to_string(symbols.lookup(value).stack_position).c_str()
            }});
        } break;

        case parse_kind::PUTS: {
            auto data = std::get<puts_t>(second(node));
            add_fields(3);

            runa_push_field(runa, (char*) "addr",
            RunaValueFFI { runa_string, RunaValueData { .string = data.str.c_str() }});

            runa_push_field(runa, (char*) "length",
            RunaValueFFI { runa_integer, RunaValueData { .integer = data.length }});

            runa_push_field(runa, (char*) "fn",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) function_id }});
        } break;

        case parse_kind::ALLOC: {
            auto [identifier, type] = std::get<declaration<symbol>>(second(node));
            SPOS to = SPOS::from(symbols, type);
            symbols.add(identifier, to);
        } break;

        case parse_kind::COMPTIMNE: {
            auto data = std::get<std::string>(second(node));
            add_field();
            runa_push_field(runa, (char*) "instruction",
            RunaValueFFI { runa_string, RunaValueData { .string = data.c_str() }});
        } break;

        case parse_kind::STORE: {
            auto store = std::get<storage>(second(node));

            SPOS dest = symbols.lookup(store.identifier);

            add_fields(3);
            runa_push_field(runa, (char*) "dest",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) dest.stack_position }});

            bool constant_string = false;
            for( auto& [name, _] : strings_stack.top() ) {
                if( name == store.value ) constant_string = true;
            }

            runa_push_field(runa, (char*) "typeof",
            RunaValueFFI { runa_integer, RunaValueData {
                .integer = (size_t) (
                    constant_string
                    ? 100
                    : is_number(store.value)
                    ? 200
                    : is_identifier(store.value)
                    ? 300
                    : 000
                )
            }});

            auto src_value = RunaValueFFI{};
            if( constant_string ) {
                std::string identifier = ".fn" + std::to_string(function_id) + "." + store.value;
                src_value =  RunaValueFFI {
                    runa_string,
                    RunaValueData { .string = identifier.c_str() }
                };

                runa_push_field(runa, (char*) "src", src_value);
                break;
            }

            if( is_number(store.value) ) {
                src_value = RunaValueFFI {
                    runa_integer,
                    RunaValueData { .integer = std::stoull(store.value) }
                };
            }

            if( is_identifier(store.value) ) {
                auto addr = symbols.lookup(store.value).stack_position;
                src_value = RunaValueFFI {
                    runa_integer,
                    RunaValueData { .integer = (size_t) addr }
                };
            }

            runa_push_field(runa, (char*) "src", src_value);
        } break;

        case parse_kind::LOAD: {
            auto [identifier, load] = std::get<declaration<std::string>>(second(node));
            symbols.add(identifier, symbols.lookup(load));
        } break;

        case parse_kind::CALL: {
            call_t call;

            if( std::holds_alternative<call_t>(second(node)) ) call = std::get<call_t>(second(node));

            auto argument = [&](int index, int tag, std::string value, int bits = 0) {
                runa_push_field(runa, (char*) "index",
                RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) index } });
                add_field();

                switch(tag) {
                    case runa_integer: {
                        add_fields(2);

                        runa_push_field(runa, (char*) "value",
                        RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) std::stol(value) } });

                        runa_push_field(runa, (char*) "typeof",
                        RunaValueFFI { runa_string, RunaValueData { .string = (char*) "integer" } });
                    } break;

                    case whatever: {
                        add_fields(3);

                        runa_push_field(runa, (char*) "value",
                        RunaValueFFI { runa_string, RunaValueData { .string = value.c_str() } });

                        runa_push_field(runa, (char*) "typeof",
                        RunaValueFFI { runa_string, RunaValueData { .string = (char*) "whatever" } });

                        runa_push_field(runa, (char*) "bits",
                        RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) bits } });
                    } break;
                }

                runa_spawn_function(runa, (char*) "argument", (runa_callback)cap);
            };

            int i = 0;
            reset_fields();
            for( std::string arg : call.args ) {
                if( is_number(arg) ) { argument(i++, runa_integer, arg); }
                if( is_identifier(arg) ) {
                    auto s = symbols.lookup(arg);
                    argument(i++, whatever, std::to_string(s.stack_position), s.data.bytes * 8);
                }
            }
            reset_fields();
            add_fields(2);

            runa_push_field(runa, (char*) "kind",
            RunaValueFFI { runa_integer, RunaValueData { .integer = parse_kind::CALL }});

            runa_push_field(runa, (char*) "identifier",
            RunaValueFFI { runa_string, RunaValueData { .string = call.identifier.c_str() } });
        }
    }
}
