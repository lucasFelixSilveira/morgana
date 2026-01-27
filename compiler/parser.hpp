#pragma once

#include <iostream>
#include <linux/limits.h>
#include <sstream>
#include <string>
#include <tuple>
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

using ParseResult = std::tuple<ParseResultKind, std::variant<
    desconstructor,
    function,
    ret,
    call,
    declaration<gpio>
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

        // Just a desconstructor parser macro defined in `./parser/descontructor.hpp`
        // where i can just place here, and make it work. I dont wanna the desconstructor
        // parsing code here, 'cause i wanna a clean code. And it is not a clean code
        DESCONSTRUCTOR;

        // Just a function parser macro defined in `./parser/function.hpp`
        // where i can just place here, and make it work. I dont wanna the function
        // parsing code here, 'cause i wanna a clean code. And it is not a clean code
        FUNCTION;

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

                call c(name, args, false);
                results.push_back({ ParseResultKind::Call, c });
            } continue;

            default:

            // Now we need parse all kinds of declaration nodes
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
                    } continue;

                    default: CompilerOutputs::Fatal("Unknown instruction (" + instruction + ")");
                }
            }

            continue;
        }
    }

    return results;
}
