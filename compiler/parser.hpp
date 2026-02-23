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
#include "parser/contexts.hpp"
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
#include "parser/types/integer.hpp"
#include "parser/types/bool.hpp"
#include "parser/types/validator.hpp"

std::vector<std::string> mocks;
int ctx;

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
    declaration<symbol>
>>;

using ParseResults = std::vector<ParseResult>;

std::string getnext(std::vector<std::string>& tokens, int& i) {
    if( i >= tokens.size() ) return "";
    return tokens[i++];
}

void sys_err(const std::string& instruction, const std::string& value, const symbol& data, bool cbid = true) {
    std::string regex;
    if( std::holds_alternative<morgana_integer>(data) ) regex = std::get<morgana_integer>(data).regex;
    if( std::holds_alternative<morgana_bool>(data) ) regex = std::get<morgana_integer>(data).regex;

    CompilerOutputs::Fatal("Error When you use `" + instruction + "` the value expect need match " + regex + (cbid ? " or be a identifier" : "") + " but is " + value);
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

                symbol data = *opt_data;

                if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                function_data func_info = std::get<function_data>(data);

                j = 0;
                for( std::string& type : func_info.types ) {
                    validation data;
                    if( data = check_valid(type, args[j++], ctx); !std::get<0>(data) ) sys_err("call", args[j - 1], std::get<1>(data));
                }

                call c(name, args);
                results.push_back({ ParseResultKind::Call, c });
                i = j;
            } continue;

            case TURN_KEYWORD: {
                ctx = context::MCU_TURN_INSTRUCTION;

                i++;
                std::string identifier = getnext(tokens, i);

                auto opt_data = symbol_table.lookup(identifier);
                if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                symbol data = *opt_data;

                if(! std::holds_alternative<named_integers>(data)) CompilerOutputs::Fatal("Invalid symbol type: " + identifier);
                auto [check, pin] = std::get<named_integers>(data);

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
                        while(std::getline(ss, arg, ',')) {
                            auto n = std::remove(arg.begin(), arg.end(), ' ');
                            args.push_back(arg.substr(0, n - arg.begin()));
                        }
                    }

                    std::vector<symbol> types;
                    for( std::string& arg : args ) {
                        auto opt_data = symbol_table.lookup(arg);
                        if(! opt_data ) CompilerOutputs::Fatal("Invalid symbol: " + arg);
                        types.push_back(*opt_data);
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

                            int ipin = std::atoi(pin.c_str());
                            if(! symbol_table.insert(token, named_integers("gpio_pin", ipin)) ) CompilerOutputs::Fatal("Symbol " + token + " already defined in this scope");
                            results.push_back({ ParseResultKind::GPIO, declaration<gpio>{ token, ipin } });

                            i--;
                        } continue;

                        case READ_INSTRUCTION: {
                            if(! mcu ) CompilerOutputs::Fatal("Digital read instructions are only available for MCUs");

                            std::string identifier = getnext(tokens, i);
                            auto opt_data = symbol_table.lookup(identifier);
                            if(! opt_data ) CompilerOutputs::Fatal("Invalid GPIO symbol: " + identifier);

                            symbol data = *opt_data;

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

                            symbol data = *opt_data;
                            if(! first(is_type(type)) ) CompilerOutputs::Fatal("Invalid symbol type: " + type);
                            results.push_back({ ParseResultKind::Alloc, declaration<symbol> { token, data } });
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

                            symbol data = *opt_data;

                            if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                            function_data func_info = std::get<function_data>(data);

                            j = 0;
                            for( std::string& type : func_info.types ) {
                                validation data;
                                if( data = check_valid(type, args[j++], ctx); !std::get<0>(data) ) {
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
