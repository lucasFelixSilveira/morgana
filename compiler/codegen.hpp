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
#include <iostream>  // Adicionado para cout
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <utility>
#include <variant>
#include <vector>

extern "C" {
#include "libs/linux/include/runa.h"
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
        std::cout << "[DEBUG] Preprocessor::push()" << std::endl;
        stack.push({});
    }

    void pop() {
        std::cout << "[DEBUG] Preprocessor::pop()" << std::endl;
        stack.pop();
    }

    void add(std::string &identifier, value val) {
        std::cout << "[DEBUG] Preprocessor::add() - identifier: " << identifier << std::endl;
        auto top = stack.top();
        top.push_back({ identifier, val });
        stack.pop();
        stack.push(top);
    }

    value lookup(std::string &identifier) {
        std::cout << "[DEBUG] Preprocessor::lookup() - identifier: " << identifier << std::endl;
        auto top = stack.top();
        for( auto &[id, val] : top ) {
            if(id == identifier) {
                std::cout << "[DEBUG] Preprocessor::lookup() - found: " << identifier << std::endl;
                return val;
            }
        }
        std::cout << "[DEBUG] Preprocessor::lookup() - NOT found: " << identifier << std::endl;
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

    Symbols() : stack_pos(0), stack() {
        std::cout << "[DEBUG] Symbols constructor" << std::endl;
    }

    void add(std::string &identifier, SPOS sym) {
        std::cout << "[DEBUG] Symbols::add() - identifier: " << identifier << ", stack_pos: " << sym.stack_position << std::endl;
        auto top = stack.top();
        stack.pop();
        top.push_back({ identifier, sym });
        stack_pos += sym.data.bytes;
        stack.push(top);
    }

    SPOS lookup(std::string &identifier) {
        std::cout << "[DEBUG] Symbols::lookup() - identifier: " << identifier << std::endl;
        auto top = stack.top();
        for( auto &[id, sp] : top ) {
            if(id == identifier) {
                std::cout << "[DEBUG] Symbols::lookup() - found: " << identifier << " at position: " << sp.stack_position << std::endl;
                return sp;
            }
        }
        CompilerOutputs::Fatal("lookup failed for " + identifier);

        return {};
    }
};

SPOS SPOS::from(Symbols& sym, symbol s) {
    std::cout << "[DEBUG] SPOS::from()" << std::endl;
    if( std::holds_alternative<morgana_integer>(s) ) {
        auto value = std::get<morgana_integer>(s);
        std::cout << "[DEBUG] SPOS::from() - integer, bits: " << value.bits << std::endl;
        return SPOS{ sym.stack_pos + (value.bits / 8), { (value.bits / 8), false } };
    };
    if( std::holds_alternative<morgana_ptr>(s) ) {
        auto value = std::get<morgana_ptr>(s);
        char bytes = 8;
        std::cout << "[DEBUG] SPOS::from() - pointer" << std::endl;
        return SPOS{ sym.stack_pos + bytes, { bytes, true } };
    };
    std::cout << "[DEBUG] SPOS::from() - unknown type" << std::endl;
    return {};
}

void morgana_push_ctx(function data) {
    std::cout << "[DEBUG] morgana_push_ctx() - function: " << data.name << std::endl;
    fn = data;
    ParseResults results = parse(data.body);
    std::cout << "[DEBUG] morgana_push_ctx() - parse results size: " << results.size() << std::endl;
    branches.push(*current);
    current = std::make_shared<iteration>(0, results);
    std::cout << "[DEBUG] morgana_push_ctx() - done" << std::endl;
}

std::string output;
std::string ident;

void tabs(Runa *runa) {
    std::cout << "[DEBUG] tabs() called" << std::endl;
    RunaValueFFI val = runa_peek_arg(runa, 0);
    size_t count = val.data.integer;
    ident = std::string(count, '\t');
    runa_value_free(val);
}

void write(Runa *runa) {
    std::cout << "[DEBUG] write() called" << std::endl;
    RunaValueFFI val = runa_peek_arg(runa, 0);
    char *str = runa_value_to_string(runa, val);
    output += std::string(str);
    runa_optional(RUNA_FREE_STRING_BY_VALUE, runa_str_free, str, val);
    runa_value_free(val);
}

void writeln(Runa *runa) {
    std::cout << "[DEBUG] writeln() called" << std::endl;
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
    std::cout << "[DEBUG] codegen() started" << std::endl;
    std::cout << "[DEBUG] Target: " << params.target << std::endl;

    std::string path = Runtime::get_executable_path("morgana");
    std::cout << "[DEBUG] Executable path: " << path << std::endl;

    #ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
    std::cout << "[DEBUG] Normalized path (Windows): " << path << std::endl;
    #endif

    auto [extensorExists, extensorPath] = Runtime::check_extensors(path, params.target);
    std::cout << "[DEBUG] Extensor exists: " << extensorExists << std::endl;
    std::cout << "[DEBUG] Extensor path: " << extensorPath << std::endl;

    if(! extensorExists ) {
        CompilerOutputs::Fatal("No extensor found for target: " + params.target);
        return "";
    }

    #ifdef _WIN32
    std::replace(extensorPath.begin(), extensorPath.end(), '/', '\\');
    std::cout << "[DEBUG] Normalized extensor path (Windows): " << extensorPath << std::endl;
    #endif

    CompilerOutputs::Info("Extensor found: " + extensorPath);
    CompilerOutputs::Info("Generating code via extensor");

    current = std::make_shared<iteration>(0, ast);
    std::cout << "[DEBUG] Current iteration created" << std::endl;

    Symbols symbols;
    std::cout << "[DEBUG] Symbols object created" << std::endl;

    Runa *runa = runa_start();
    std::cout << "[DEBUG] Runa started" << std::endl;

    runa_needs(runa, write, writeln, tabs);
    std::cout << "[DEBUG] Runa needs configured" << std::endl;

    std::vector<char> extensorPathCStr(extensorPath.begin(), extensorPath.end());
    extensorPathCStr.push_back('\0');
    std::cout << "[DEBUG] Loading extensor file: " << extensorPathCStr.data() << std::endl;

    runa_loadfile(runa, extensorPathCStr.data());

    int iteration_count = 0;
    while(true) {
        iteration_count++;
        std::cout << "[DEBUG] Iteration " << iteration_count << std::endl;

        reset_fields();
        std::cout << "[DEBUG] Fields reset" << std::endl;

        auto& [index, data] = *current;
        std::cout << "[DEBUG] Current index: " << index << ", data size: " << data.size() << std::endl;

        if( index < data.size() ) {
            std::cout << "[DEBUG] Processing node " << index << std::endl;
            index++;
            std::cout << "[DEBUG] Calling table()" << std::endl;
            table(symbols, runa, data.at(index - 1));
            std::cout << "[DEBUG] Table() completed, spawning function" << std::endl;
            runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);
            std::cout << "[DEBUG] Function spawned" << std::endl;
            continue;
        } else {
            std::cout << "[DEBUG] End of data reached" << std::endl;
            if( branches.empty() ) {
                std::cout << "[DEBUG] Branches empty, breaking loop" << std::endl;
                break;
            } else {
                std::cout << "[DEBUG] Calling epilogue and pendent_strings_make" << std::endl;
                epilogue(symbols, runa);
                pendent_strings_make(runa);
            }
            current = std::make_shared<iteration>(branches.top());
            branches.pop();
            std::cout << "[DEBUG] New current iteration set" << std::endl;
        }
    }

    std::cout << "[DEBUG] Code generation completed, output size: " << output.size() << std::endl;
    runa_free(runa);
    std::cout << "[DEBUG] Runa freed" << std::endl;

    return output;
}

void epilogue(Symbols& symbols, Runa *runa) {
    std::cout << "[DEBUG] epilogue() started" << std::endl;
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
    std::cout << "[DEBUG] epilogue() completed" << std::endl;
}

void pendent_strings_make(Runa *runa) {
    std::cout << "[DEBUG] pendent_strings_make() started" << std::endl;
    reset_fields();

    add_field();
    runa_push_field(runa, (char*) "kind",
    RunaValueFFI { runa_integer, RunaValueData { .integer = parse_kind::DATA }});
    runa_spawn_function(runa, (char*) "codegen", (runa_callback)cap);

    reset_fields();
    add_fields(3);
    for( auto& [name, value] : strings_stack.top() ) {
        std::cout << "[DEBUG] Processing string: " << name << " = " << value << std::endl;
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
    std::cout << "[DEBUG] pendent_strings_make() completed, function_id: " << function_id << std::endl;

    reset_fields();
}


void table(Symbols& symbols, Runa *runa, ParseResult& node) {
    std::cout << "[DEBUG] table() started" << std::endl;

    runa_push_field(runa, (char*) "kind", RunaValueFFI {
        runa_integer, RunaValueData { .integer = (size_t) first(node) }
    });

    add_field();

    parse_kind kind = first(node);
    std::cout << "[DEBUG] Processing kind: " << (int)kind << std::endl;

    switch(kind) {
        case parse_kind::FUNCTION: {
            std::cout << "[DEBUG] Case FUNCTION" << std::endl;
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
            std::cout << "[DEBUG] Case RET" << std::endl;
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
            RunaValueFFI { runa_string, RunaValueData { .string = value.c_str() }});
        } break;

        case parse_kind::PUTS: {
            std::cout << "[DEBUG] Case PUTS" << std::endl;
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
            std::cout << "[DEBUG] Case ALLOC" << std::endl;
            auto [identifier, type] = std::get<declaration<symbol>>(second(node));
            SPOS to = SPOS::from(symbols, type);
            symbols.add(identifier, to);
        } break;

        case parse_kind::COMPTIMNE: {
            std::cout << "[DEBUG] Case COMPTIMNE" << std::endl;
            auto data = std::get<std::string>(second(node));
            add_field();
            runa_push_field(runa, (char*) "instruction",
            RunaValueFFI { runa_string, RunaValueData { .string = data.c_str() }});
        } break;

        case parse_kind::STORE: {
            std::cout << "[DEBUG] Case STORE" << std::endl;
            auto store = std::get<storage>(second(node));

            SPOS dest = symbols.lookup(store.identifier);

            add_field();
            runa_push_field(runa, (char*) "dest",
            RunaValueFFI { runa_integer, RunaValueData { .integer = (size_t) dest.stack_position }});

            bool constant_string = false;
            for( auto& [name, _] : strings_stack.top() ) {
                if( name == store.value ) constant_string = true;
            }

            if( constant_string ) {
                std::string identifier = ".fn" + std::to_string(function_id) + "." + store.value;
                add_field();
                runa_push_field(runa, (char*) "src",
                RunaValueFFI { runa_string, RunaValueData { .string = identifier.c_str() } } );
                break;
            }
        } break;

        case parse_kind::LOAD: {
            std::cout << "[DEBUG] Case LOAD" << std::endl;
            auto [identifier, load] = std::get<declaration<std::string>>(second(node));
            symbols.add(identifier, symbols.lookup(load));
        } break;

        case parse_kind::CALL: {
            std::cout << "[DEBUG] Case CALL" << std::endl;
            call_t call;

            if( std::holds_alternative<call_t>(second(node)) ) call = std::get<call_t>(second(node));

            auto argument = [&](int index, int tag, std::string value, int bits = 0) {
                std::cout << "[DEBUG] argument() - index: " << index << ", tag: " << tag << ", value: " << value << std::endl;
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
                std::cout << "[DEBUG] Processing argument: " << arg << std::endl;
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
    std::cout << "[DEBUG] table() completed" << std::endl;
}
