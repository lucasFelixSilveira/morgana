#pragma once

#include <algorithm>
#include <linux/limits.h>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
#include <regex>

#include "main.hpp"
#include "compiler_outputs.hpp"
#include "parser/make_it_integer.hpp"
#include "parser/checkout.hpp"
#include "parser/desconstructor.hpp"
#include "parser/function.hpp"
#include "parser/call.hpp"
#include "parser/keywords.hpp"
#include "parser/declaration.hpp"
#include "parser/mcu/gpio.hpp"
#include "parser/symbols.hpp"
#include "parser/type.hpp"

std::vector<std::string> mocks;

enum ParseResultKind {
    // Functions nodes
    Desconstructor,
    Function,
    Ret,
    Call,

    // MCUs nodes
    GPIO
};


enum symbolKind { GPIO_PIN };

using symbol_data = std::variant<morgana_types, morgana_subtypes, function_data, std::tuple<std::string, int>>;
using symbols = std::unordered_map<std::string, symbol_data>;
std::stack<symbols> symstack;

symbols global = {
    {"i8",  morgana_types { .value = type::common(false, "i8"),  .regex = "[0-9]+" }},
    {"i16", morgana_types { .value = type::common(false, "i16"), .regex = "[0-9]+" }},
    {"i32", morgana_types { .value = type::common(false, "i32"), .regex = "[0-9]+" }},
    {"i64", morgana_types { .value = type::common(false, "i64"), .regex = "[0-9]+" }},

    {"gpio_pin", morgana_subtypes { .instruction = symbolKind::GPIO_PIN, .identifier = "gpio_pin", .real_one = type::common(false, "i16") }},
};

using validation = std::tuple<bool,symbol_data>;
validation check_valid(std::string type, std::string value) {
    symbol_data data = global.at(type);
    if( std::holds_alternative<morgana_types>(data) ) {
        morgana_types t = std::get<morgana_types>(data);

        if( std::regex_match(value, std::regex(t.regex)) ) return { true, data };

        if( is_identifier(value) ) {
            if( global.count(value) == 0 ) return { false, data };
            symbol_data data = global.at(value);

            if( std::holds_alternative<std::tuple<std::string, int>>(data) ) {
                auto [what, number] = std::get<std::tuple<std::string, int>>(data);
                return { what == type, data };
            }

            return { false, data };
        }
    }

    if( std::holds_alternative<morgana_subtypes>(data) ) {
        morgana_subtypes t = std::get<morgana_subtypes>(data);

        if( is_identifier(value) ) {
            if( global.count(value) == 0 ) return { false, data };
            symbol_data data = global.at(value);

            if( std::holds_alternative<std::tuple<std::string, int>>(data) ) {
                auto [what, number] = std::get<std::tuple<std::string, int>>(data);
                return { what == t.identifier, data };
            }

            return { false, data };
        }
    }

    return { false, data };
}


using ParseResult = std::tuple<ParseResultKind, std::variant<
    desconstructor,
    function,
    ret,
    call,
    declaration<gpio>,
    declaration<call>
>>;

using ParseResults = std::vector<ParseResult>;

std::string getnext(std::vector<std::string>& tokens, int& i) {
    if( i >= tokens.size() ) return "";
    return tokens[i++];
}

void sys_err(std::string instruction, std::string value, symbol_data data, bool cbid = true) {
    if( std::holds_alternative<morgana_types>(data) ) {
        auto type = std::get<morgana_types>(data);
        CompilerOutputs::Fatal("Error When you use `" + instruction + "` the value expect need match " + type.regex + (cbid ? " or be a identifier" : "") + " but is " + value);
    }
};

bool fparse = true;
ParseResults parse(std::vector<std::string>& tokens) {
    bool mcu = (params.target == "xtensa" ? true : (params.target == "avr"));

    fparse = false;
    ParseResults results = {};

    for( int i = 0; i < tokens.size(); i++ ) {
        std::string token = tokens[i], next = "";
        if( (i + 1) < tokens.size() ) next = tokens[i + 1];

        auto checkout = make_it_integer(token);
        switch(checkout) {
            // This is just keywords. All "keywords" are just a monostate data.
            // this means: All data who we need is the keyword
            NON_COMPLEX_NODE(RET_KEYWORD, ParseResultKind::Ret);

            // This keywords bellow can't be just passed without more values,
            // they need to be parsed with more data and validation
            case CALL_KEYWORD: {
                std::string name;
                std::vector<std::string> args;
                std::string callStr;

                int j = i + 1;
                for(; j < tokens.size(); j++ ) {
                    callStr += tokens[j] + " ";
                    if( tokens[j].find(')') != std::string::npos ) break;
                }

                if( j >= tokens.size() ) CompilerOutputs::Fatal("Bad call instruction");

                callStr = std::regex_replace(callStr, std::regex("\\s+"), "");
                std::regex r(R"((\w+)\((.*)\))");

                std::smatch match;
                if( std::regex_search(callStr, match, r) ) {
                    name = match[1].str();

                    std::stringstream ss(match[2].str());
                    std::string arg;
                    while(std::getline(ss, arg, ',')) args.push_back(arg);
                }

                if( global.count(name) == 0 ) CompilerOutputs::Fatal("Invalid symbol: " + name);
                symbol_data data = global.at(name);

                if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                function_data func_info = std::get<function_data>(data);

                j = 0;
                for( std::string& type : func_info.types ) {
                    validation data;
                    if( data = check_valid(type, args[j++]); !first(data) ) sys_err("call", args[j - 1], second(data));
                }

                call c(name, args);
                results.push_back({ ParseResultKind::Call, c });
            } continue;

            default:

            // Now we need parse all kinds of declaration nodes
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

                i = j - 1;

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

                std::vector<morgana_paramters> types;
                for( std::string& arg : args) {
                    if( global.count(arg) == 0 ) CompilerOutputs::Fatal("Invalid symbol: " + arg);

                    symbol_data data = global[arg];
                    if( std::holds_alternative<morgana_types>(data) ) goto function_ignore_ifs;
                    if( std::holds_alternative<morgana_subtypes>(data) ) goto function_ignore_ifs;

                    function_ignore_ifs: {};
                    if( std::holds_alternative<morgana_types>(data) ) types.push_back(std::get<morgana_types>(data));
                    if( std::holds_alternative<morgana_subtypes>(data) ) types.push_back(std::get<morgana_subtypes>(data));
                }

                std::vector<std::string> body;
                unsigned int into = 0;
                while(tokens[i] != "}" && into < 1) {
                    if( tokens[i] == "{" ) into++;
                    else if( tokens[i] == "}" ) into--;
                    body.push_back(tokens[i++]);
                }

                global.insert({ name, function_data { .types = args } });
                function f(name, types, std::get<type>(info), body);
                results.push_back({ ParseResultKind::Function, f });

                continue;
            }

            if( is_identifier(token) && next == "=" ) {
                i += 2;
                std::string instruction = getnext(tokens, i);
                auto checkout = make_it_integer(instruction);

                switch(checkout) {

                    // GPIO instructions for MCUs
                    case GPIO_INSTRUCTION: {
                        if(! mcu ) CompilerOutputs::Fatal("GPIO instructions are only available for MCUs");

                        std::string pin = getnext(tokens, i);
                        if(! is_number(pin) ) CompilerOutputs::Fatal("Enter with a valid GPIO. `id = gpio 12`");

                        global.insert({ token, std::make_tuple("gpio_pin", std::atoi(pin.c_str())) });
                        results.push_back({ ParseResultKind::GPIO, declaration<gpio> { token, std::atoi(pin.c_str()) } });

                        i--;
                    } continue;

                    case CALL_KEYWORD: {
                        std::string name;
                        std::vector<std::string> args;
                        std::string callStr;

                        int j = i + 1;
                        for(; j < tokens.size(); j++ ) {
                            callStr += getnext(tokens, i) + " ";
                            if( tokens[j].find(')') != std::string::npos ) break;
                        }

                        std::regex r("([a-zA-Z0-9_]+)\\(([^)]*)\\)");
                        std::smatch match;
                        if( std::regex_search(callStr, match, r) ) {
                            name = match[1].str();

                            std::stringstream ss(match[2].str());
                            std::string arg;
                            while(std::getline(ss, arg, ',')) args.push_back(arg);
                        }

                        if( global.count(name) == 0 ) CompilerOutputs::Fatal("Invalid symbol: " + name);
                        symbol_data data = global.at(name);

                        if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                        function_data func_info = std::get<function_data>(data);

                        j = 0;
                        for( std::string& type : func_info.types ) {
                            validation data;
                            if( data = check_valid(type, args[j++]); !first(data) ) sys_err("call", args[j - 1], second(data));
                        }

                        i--;
                        call c(name, args);
                        results.push_back({ ParseResultKind::Call, declaration<call> { token, c } });
                    } continue;

                    default: CompilerOutputs::Fatal("Unknown instruction (" + instruction + ")");
                }
            }

            continue;
        }
    }

    return results;
}
