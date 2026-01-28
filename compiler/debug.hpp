#pragma once
#include "compiler_outputs.hpp"
#include "parser.hpp"
#include <iostream>
#include <unordered_map>
#include <variant>

#include "parser/declaration.hpp"
#include "parser/function.hpp"
#include "parser/symbols.hpp"

enum ParseNodeKind {
    MCU,
    CPU,
    Common
};

std::unordered_map<ParseResultKind, std::tuple<ParseNodeKind, std::string>> token_names = {
    { ParseResultKind::Function, std::make_tuple(Common, "Function") },
    { ParseResultKind::Call, std::make_tuple(Common, "Call") },
    { ParseResultKind::Ret, std::make_tuple(Common, "Ret") },
    { ParseResultKind::GPIO, std::make_tuple(MCU, "GPIO") },
};

void debug_print(ParseResults results) {
    for( const ParseResult& token : results ) {
        auto [kind, name] = token_names[first(token)];
        if( kind == ParseNodeKind::MCU )    std::cout << Colorizer::BOLD_BLUE << '[' << name << ']' << Colorizer::RESET << ":\n";
        if( kind == ParseNodeKind::CPU )    std::cout << Colorizer::BOLD_RED << '[' << name << ']' << Colorizer::RESET << ":\n";
        if( kind == ParseNodeKind::Common ) std::cout << Colorizer::BOLD_YELLOW << '[' << name << ']' << Colorizer::RESET << ":\n";

        switch(first(token)) {
            case ParseResultKind::Function: {
                auto data = std::get<function>(second(token));
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "identifier" << Colorizer::RESET << ": " << data.name << "\n";
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "body" << Colorizer::RESET << ": std::vector<std::string>(" << data.body.size() << ")\n";
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "arguments" << Colorizer::RESET << ": std::vector<type>(" << data.argst.size() << ")\n";
                for( const auto& argt : data.argst ) {
                    if( std::holds_alternative<morgana_types>(argt)) std::cout << "   " << Colorizer::DARK_GREY << "└─ " << Colorizer::YELLOW << std::get<morgana_types>(argt).value.value << "\n";
                    if( std::holds_alternative<morgana_subtypes>(argt)) std::cout << "   " << Colorizer::DARK_GREY << "└─ " << Colorizer::BOLD_CYAN << std::get<morgana_subtypes>(argt).identifier << "\n";
                }
                std::cout << "\n";
            } break;

            case ParseResultKind::Call: {
                call data =
                    ((std::holds_alternative<declaration<call>>(second(token))) ? second(std::get<declaration<call>>(second(token)))
                                                                                : std::get<call>(second(token)));

                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "function" << Colorizer::RESET << ": " << data.func << "\n";
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "arguments" << Colorizer::RESET << ": std::vector<std::string>(" << data.args.size() << ")\n";
                std::cout << "\n";
            } break;

            case ParseResultKind::GPIO: {
                auto data = std::get<declaration<gpio>>(second(token));
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "identifier" << Colorizer::RESET << ": " << first(data) << "\n";
                std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::GREEN << "pin" << Colorizer::RESET << ": " << second(data) << "\n";
                std::cout << "\n";
            } break;
        }
    }
}
