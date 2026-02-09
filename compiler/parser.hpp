#pragma once

#include <algorithm>
#include <cstdlib>
#include <linux/limits.h>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
#include <regex>
#include <stack>

#include "main.hpp"
#include "compiler_outputs.hpp"
#include "parser/comp.hpp"
#include "parser/make_it_integer.hpp"
#include "parser/checkout.hpp"
#include "parser/loop.hpp"
#include "parser/function.hpp"
#include "parser/call.hpp"
#include "parser/keywords.hpp"
#include "parser/turn.hpp"
#include "parser/declaration.hpp"
#include "parser/mcu/read.hpp"
#include "parser/mcu/gpio.hpp"
#include "parser/symbols.hpp"
#include "parser/type.hpp"

std::vector<std::string> mocks;

enum ParseResultKind {
    // Functions nodes
    Desconstructor     = 10,
    Function           = 11,
    Call               = 12,
    Ret                = 13,

    Wait               = 20,
    WaitMS             = 21,

    Label              = 30,
    Branch             = 31,
    BranchNotEqualZero = 32,

    Alloc              = 40,
    Store              = 41,
    Load               = 42,

    Loop               = 700,

    // MCUs nodes
    GPIO               = 1001,
    Turn               = 1002,
    Read               = 1003,
};

enum symbolKind { GPIO_PIN };

using symbol_data = std::variant<morgana_types, morgana_subtypes, function_data, std::tuple<std::string, int>>;

class SymbolTable {
private:
    std::stack<std::unordered_map<std::string, symbol_data>> scopes;

public:
    SymbolTable() {
        enter_scope();
        current_scope().insert({
            {"i8",  morgana_types { .value = type::common(false, "i8"),  .regex = "[0-9]+" }},
            {"i16", morgana_types { .value = type::common(false, "i16"), .regex = "[0-9]+" }},
            {"i32", morgana_types { .value = type::common(false, "i32"), .regex = "[0-9]+" }},
            {"i64", morgana_types { .value = type::common(false, "i64"), .regex = "[0-9]+" }},
            {"gpio_pin", morgana_subtypes { .instruction = symbolKind::GPIO_PIN, .identifier = "gpio_pin", .real_one = type::common(false, "i16") }},
        });
    }

    void enter_scope() {
        scopes.push(std::unordered_map<std::string, symbol_data>());
    }

    // Sai do escopo atual
    void exit_scope() {
        if( scopes.size() > 1 ) scopes.pop();
    }

    std::unordered_map<std::string, symbol_data>& current_scope() {
        return scopes.top();
    }

    bool insert(std::string& name, symbol_data data) {
        auto& scope = current_scope();
        if (scope.find(name) != scope.end()) return false;
        scope[name] = data;
        return true;
    }

    std::optional<symbol_data> lookup(std::string name) {
        auto temp_stack = scopes;

        while(!temp_stack.empty()) {
            auto& scope = temp_stack.top();
            auto it = scope.find(name);
            if( it != scope.end() ) return it->second;
            temp_stack.pop();
        }
        return std::nullopt;
    }

    std::optional<symbol_data> lookup_current(const std::string& name) {
        auto& scope = current_scope();
        auto it = scope.find(name);
        if (it != scope.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool exists(std::string& name) {
        return lookup(name).has_value();
    }

    bool exists_in_current(std::string& name) {
        return lookup_current(name).has_value();
    }

    bool remove(std::string& name) {
        auto& scope = current_scope();
        return scope.erase(name) > 0;
    }

    size_t scope_level() const {
        return scopes.size() - 1;
    }
};

// Substitui a tabela de símbolos global por uma instância da nova classe
SymbolTable symbol_table;

using validation = std::tuple<bool, symbol_data>;

validation check_valid(const std::string& type, const std::string& value) {
    auto opt_data = symbol_table.lookup(type);
    if(! opt_data ) return { false, symbol_data{} };

    symbol_data data = *opt_data;

    if( std::holds_alternative<morgana_types>(data) ) {
        morgana_types t = std::get<morgana_types>(data);

        if( std::regex_match(value, std::regex(t.regex)) ) return { true, data };

        if( is_identifier(value) ) {
            auto val_data = symbol_table.lookup(value);
            if(! val_data ) return { false, data };

            if( std::holds_alternative<std::tuple<std::string, int>>(*val_data) ) {
                auto [what, number] = std::get<std::tuple<std::string, int>>(*val_data);
                return { what == type, data };
            }

            return { false, data };
        }
    }

    if (std::holds_alternative<morgana_subtypes>(data)) {
        morgana_subtypes t = std::get<morgana_subtypes>(data);

        if (is_identifier(value)) {
            auto val_data = symbol_table.lookup(value);
            if (!val_data) {
                return { false, data };
            }

            if (std::holds_alternative<std::tuple<std::string, int>>(*val_data)) {
                auto [what, number] = std::get<std::tuple<std::string, int>>(*val_data);
                return { what == t.identifier, data };
            }

            return { false, data };
        }
    }

    return { false, data };
}

using ParseResult = std::tuple<ParseResultKind, std::variant<
    std::string,
    int,
    function,
    ret,
    call,
    turn,
    loop,
    brnez,
    declaration<gpio>,
    declaration<call>,
    declaration<mcu_read>,
    declaration<symbol_data>
>>;

using ParseResults = std::vector<ParseResult>;

std::string getnext(std::vector<std::string>& tokens, int& i) {
    if( i >= tokens.size() ) return "";
    return tokens[i++];
}

void sys_err(const std::string& instruction, const std::string& value, const symbol_data& data, bool cbid = true) {
    if( std::holds_alternative<morgana_types>(data) ) {
        auto type = std::get<morgana_types>(data);
        CompilerOutputs::Fatal("Error When you use `" + instruction + "` the value expect need match " + type.regex + (cbid ? " or be a identifier" : "") + " but is " + value);
    }
}

bool fparse = true;

ParseResults parse(std::vector<std::string>& tokens) {
    bool mcu = (params.target == "xtensa" ? true : (params.target == "avr"));

    fparse = false;
    ParseResults results = {};

    // Contexto de função atual (para gerenciar escopos)
    bool in_function = false;

    for( int i = 0; i < tokens.size(); i++ ) {
        std::string token = tokens[i], next = "";
        if( (i + 1) < tokens.size() ) next = tokens[i + 1];

        auto checkout = make_it_integer(token);
        switch(checkout) {
            case DEFINE_LABEL: {
                std::string label = token.substr(2, token.size() - 4);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Label statement needs a identifier");
                results.push_back({ ParseResultKind::Label, label });
            } continue;

            case RET_KEYWORD: {
                if(! in_function ) CompilerOutputs::Fatal("Return statement outside of function");
                results.push_back({ ParseResultKind::Ret, std::monostate() });
            } continue;

            case LOOP_KEYWORD: {
                i++;
                if( getnext(tokens, i) != "{" ) CompilerOutputs::Fatal("Loop statement needs a code-block");

                std::vector<std::string> body;
                for(; i < tokens.size(); i++ ) {
                    body.push_back(tokens[i]);
                    if( tokens[i] == "}" ) break;
                }

                if( i >= tokens.size() ) CompilerOutputs::Fatal("Bad loop instruction");

                results.push_back({ ParseResultKind::Loop, loop{ .body = body } });
            } continue;

            case WAIT_KEYWORD:
            case WAITMS_KEYWORD: {
                i++;
                std::string number = getnext(tokens, i);
                if(! is_number(number) ) CompilerOutputs::Fatal("Wait statement needs a numeric complement");
                results.push_back({
                    (checkout == WAIT_KEYWORD ? ParseResultKind::Wait : ParseResultKind::WaitMS),
                    atoi(number.c_str())
                });
                i--;
            } continue;

            case BRANCH_NOT_EQUAL_ZERO_KEYWORD: {
                i++;
                std::string identifier = getnext(tokens, i);
                if(! is_identifier(identifier) ) CompilerOutputs::Fatal("Branch not equal zero statement needs an identifier");

                std::string label = getnext(tokens, i);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Branch not equal zero statement also needs an identifier on the second argument");

                results.push_back({
                    ParseResultKind::BranchNotEqualZero,
                    brnez { identifier, label }
                });
                i--;
            } continue;

            case BRANCH_KEYWORD: {
                i++;
                std::string label = getnext(tokens, i);
                if(! is_identifier(label) ) CompilerOutputs::Fatal("Label statement needs an identifier");
                results.push_back({ ParseResultKind::Branch, label });
                i--;
            } continue;

            case CALL_KEYWORD: {
                std::string name;
                std::vector<std::string> args;
                std::string callStr;

                int j = i + 1;
                for(; j < tokens.size(); j++ ) {
                    callStr += tokens[j] + " ";
                    if (tokens[j].find(')') != std::string::npos) break;
                }

                if( j >= tokens.size() ) CompilerOutputs::Fatal("Bad call instruction");

                callStr = std::regex_replace(callStr, std::regex("\\s+"), "");
                std::regex r(R"((\w+)\((.*)\))");

                std::smatch match;
                if( std::regex_search(callStr, match, r) ) {
                    name = match[1].str();

                    std::stringstream ss(match[2].str());
                    std::string arg;
                    while (std::getline(ss, arg, ',')) args.push_back(arg);
                }

                auto opt_data = symbol_table.lookup(name);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + name);

                symbol_data data = *opt_data;

                if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                function_data func_info = std::get<function_data>(data);

                j = 0;
                for( std::string& type : func_info.types ) {
                    validation data;
                    if( data = check_valid(type, args[j++]); !std::get<0>(data) ) sys_err("call", args[j - 1], std::get<1>(data));
                }

                call c(name, args);
                results.push_back({ ParseResultKind::Call, c });
                i = j;
            } continue;

            case TURN_KEYWORD: {
                i++;
                std::string identifier = getnext(tokens, i);

                auto opt_data = symbol_table.lookup(identifier);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                symbol_data data = *opt_data;

                if(! std::holds_alternative<std::tuple<std::string, int>>(data)) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);
                auto [check, pin] = std::get<std::tuple<std::string, int>>(data);

                if( check != "gpio_pin" ) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);

                std::string toggle = getnext(tokens, i);
                if( toggle != "on" && toggle != "off" ) CompilerOutputs::Fatal("Invalid complement, you just can put the pin " + identifier + " as on/off");

                turn t(pin, toggle == "on");
                results.push_back({ ParseResultKind::Turn, t });
                i--;
            } continue;

            default: {
                auto [is, info] = is_type(token);
                if( is && is_identifier(next) ) {
                    std::string name;
                    std::vector<std::string> args;

                    std::string callStr;
                    int j = i + 1;
                    for(; j < tokens.size(); ) {
                        callStr += getnext(tokens, j) + " ";
                        if( tokens[j-1].find(')') != std::string::npos ) break;
                    }
                    i = j + 1;

                    std::regex r("([a-zA-Z0-9_]+)\\(([^)]*)[\\)]?");
                    std::smatch match;
                    if( std::regex_search(callStr, match, r) ) {
                        name = match[1].str();

                        std::stringstream ss(match[2].str());
                        std::string arg;
                        while (std::getline(ss, arg, ',')) {
                            auto n = std::remove(arg.begin(), arg.end(), ' ');
                            args.push_back(arg.substr(0, n - arg.begin()));
                        }
                    }

                    std::vector<morgana_paramters> types;
                    for( std::string& arg : args ) {
                        auto opt_data = symbol_table.lookup(arg);
                        if (!opt_data) CompilerOutputs::Fatal("Invalid symbol: " + arg);

                        symbol_data data = *opt_data;
                        if( std::holds_alternative<morgana_types>(data)) types.push_back(std::get<morgana_types>(data));
                        else if (std::holds_alternative<morgana_subtypes>(data)) types.push_back(std::get<morgana_subtypes>(data));
                    }

                    std::vector<std::string> body;
                    unsigned int into = 0;

                    for( auto& arg : args ) {
                        auto opt_param_type = symbol_table.lookup(arg);
                        if( opt_param_type ) symbol_table.insert(arg, *opt_param_type);
                    }

                    while(true) {
                        if( tokens[i] == "{" ) into++;
                        else if( tokens[i] == "}" && into == 0 ) break;
                        else if( tokens[i] == "}" ) into--;
                        body.push_back(tokens[i++]);
                    }

                    if(! symbol_table.insert(name, function_data{ .types = args }) ) CompilerOutputs::Fatal("Function " + name + " already defined");

                    function f(name, types, std::get<type>(info), body);
                    results.push_back({ ParseResultKind::Function, f });

                    continue;
                }

                if( is_identifier(token) && next == "=" ) {
                    i += 2;
                    std::string instruction = getnext(tokens, i);
                    auto checkout = make_it_integer(instruction);

                    switch (checkout) {
                        case GPIO_INSTRUCTION: {
                            if(! mcu ) CompilerOutputs::Fatal("GPIO instructions are only available for MCUs");

                            std::string pin = getnext(tokens, i);
                            if(! is_number(pin) ) CompilerOutputs::Fatal("Enter with a valid GPIO. `id = gpio 12`");

                            if(! symbol_table.insert(token, std::make_tuple("gpio_pin", std::atoi(pin.c_str()))) ) CompilerOutputs::Fatal("Symbol " + token + " already defined in this scope");

                            results.push_back({ ParseResultKind::GPIO, declaration<gpio>{ token, std::atoi(pin.c_str()) } });
                            i--;
                        } continue;

                        case READ_INSTRUCTION: {
                            if(! mcu ) CompilerOutputs::Fatal("Digital read instructions are only available for MCUs");

                            std::string identifier = getnext(tokens, i);
                            auto opt_data = symbol_table.lookup(identifier);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                            symbol_data data = *opt_data;

                            if(! std::holds_alternative<std::tuple<std::string, int>>(data)) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);
                            auto [check, pin] = std::get<std::tuple<std::string, int>>(data);

                            if( check != "gpio_pin" ) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);

                            results.push_back({ ParseResultKind::Read, declaration<mcu_read>{ token, mcu_read { pin, is_digital(pin) } } });
                            i--;
                        } continue;

                        case ALLOC_INSTRUCTION: {
                            std::string type = getnext(tokens, i);

                            auto opt_data = symbol_table.lookup(type);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + type);

                            symbol_data data = *opt_data;

                            if(
                               !std::holds_alternative<morgana_types>(data)
                            && !std::holds_alternative<morgana_subtypes>(data)
                            )  CompilerOutputs::Fatal("Invalid symbol type: " + type);

                            results.push_back({ ParseResultKind::Alloc, declaration<symbol_data> { token, data } });
                            i--;
                        } continue;

                        case CALL_KEYWORD: {
                            std::string name;
                            std::vector<std::string> args;
                            std::string callStr;

                            int j = i + 1;
                            for(; j < tokens.size(); j++ ) {
                                callStr += getnext(tokens, i) + " ";
                                if (tokens[j].find(')') != std::string::npos) break;
                            }

                            std::regex r("([a-zA-Z0-9_]+)\\(([^)]*)\\)");
                            std::smatch match;
                            if( std::regex_search(callStr, match, r) ) {
                                name = match[1].str();

                                std::stringstream ss(match[2].str());
                                std::string arg;
                                while (std::getline(ss, arg, ',')) args.push_back(arg);
                            }

                            auto opt_data = symbol_table.lookup(name);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + name);

                            symbol_data data = *opt_data;

                            if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                            function_data func_info = std::get<function_data>(data);

                            j = 0;
                            for( std::string& type : func_info.types ) {
                                validation data;
                                if( data = check_valid(type, args[j++]); !std::get<0>(data) ) {
                                    sys_err("call", args[j - 1], std::get<1>(data));
                                }
                            }

                            i--;
                            call c(name, args);
                            results.push_back({ ParseResultKind::Call, declaration<call>{ token, c } });
                        } continue;

                        default: CompilerOutputs::Fatal("Unknown instruction (" + instruction + ")");
                    }
                }

                continue;
            }
        }
    }

    return results;
}
