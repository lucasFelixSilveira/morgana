#pragma once

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


using symbol_data = std::variant<morgana_types, function_data>;
using symbols = std::unordered_map<std::string, symbol_data>;
std::stack<symbols> symstack;

symbols global = {
    {"i8",  morgana_types { .value = type::common(false, "i8"),  .regex = "[0-9]+" }},
    {"i16", morgana_types { .value = type::common(false, "i16"), .regex = "[0-9]+" }},
    {"i32", morgana_types { .value = type::common(false, "i32"), .regex = "[0-9]+" }},
    {"i64", morgana_types { .value = type::common(false, "i64"), .regex = "[0-9]+" }},
};

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

ParseResults parse(std::vector<std::string>& tokens) {
    bool mcu = (params.target == "xtensa" ? true : (params.target == "avr"));
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
                symbol_data data = global[name];

                if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                function_data func_info = std::get<function_data>(data);

                j = 0;
                for( std::string& type : func_info.types ) {
                    symbol_data data = global[type];
                    if( std::holds_alternative<morgana_types>(data) ) {
                        morgana_types t = std::get<morgana_types>(data);
                        if(! std::regex_match(args[j++], std::regex(t.regex)) ) {
                            CompilerOutputs::Fatal("When you use `call` for " + name + ", your " + std::to_string(j) + " argument is not of type `" + type + "`. The value should match the regex `" + t.regex + "` and you did enter `" + args[j-1] + "`");
                        }
                    }
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

                std::vector<type> types;
                for( std::string& arg : args) {
                    if( global.count(arg) == 0 ) CompilerOutputs::Fatal("Invalid symbol: " + arg);

                    symbol_data data = global[arg];
                    if(! std::holds_alternative<morgana_types>(data) ) CompilerOutputs::Fatal("Invalid type: " + arg);

                    types.push_back(std::get<morgana_types>(data).value);
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
                        symbol_data data = global[name];

                        if(! std::holds_alternative<function_data>(data) ) CompilerOutputs::Fatal("Invalid function: " + name);
                        function_data func_info = std::get<function_data>(data);

                        j = 0;
                        for( std::string& type : func_info.types ) {
                            symbol_data data = global[type];
                            if( std::holds_alternative<morgana_types>(data) ) {
                                morgana_types t = std::get<morgana_types>(data);
                                if(! std::regex_match(args[j++], std::regex(t.regex)) ) {
                                    CompilerOutputs::Fatal("When you use `call` for " + name + ", your " + std::to_string(j) + " argument is not of type `" + type + "`. The value should match the regex `" + t.regex + "` and you did enter `" + args[j-1] + "`");
                                }
                            }
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
