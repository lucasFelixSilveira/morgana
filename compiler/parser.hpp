#pragma once

#include "compiler_outputs.hpp"
#include "params.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

std::tuple<std::string, std::string, int> types[] = {
    { "u8", "uint8_t", 1 },   { "i8", "int8_t", 1 },
    { "u16", "uint16_t", 2 }, { "i16", "int16_t", 2 },
    { "u32", "uint32_t", 4 }, { "i32", "int32_t", 4 },
    { "u64", "uint64_t", 8 }, { "i64", "int64_t", 8 },
    { "void", "void", 0 }
};

std::string string_cpp(const std::string common) {
    for( auto type : types ) {
        if( common == std::get<0>(type) ) return std::get<1>(type);
    }

    return "";
};

std::vector<std::tuple<std::string, std::string>> basics = {};
// std::vector<std::string> alias = {};



struct type {
public:
    enum radical { Common, Vector, Array };
    radical kind;
    std::string value;
    int size = 0;
    bool ptr = false;

    static type common(bool ptr, std::string value) {
        return type(ptr, Common, value);
    }

    static type vector(bool ptr, std::string value) {
        return type(ptr, Vector, value);
    }

    static type array(bool ptr, std::string value, int size) {
        return type(ptr, Array, value, size);
    }

    type(bool ptr, radical kind, std::string value, int size = 0) : ptr(ptr), kind(kind), value(value), size(size) {}

    std::string string(bool optimized = false, std::string var = "") const {
        std::stringstream ss;

        if( kind == radical::Common && optimized ) {
            ss << string_cpp(value);
            if(! var.empty() ) ss << ' ';
            if( ptr ) ss << "*";
            if(! var.empty() ) ss << var;
            return ss.str();
        }

        if( kind == radical::Common ) {
            ss << string_cpp(value);
            if( ptr ) ss << "&";
            if(! var.empty() ) ss << ' ' << var;
            return ss.str();
        }

        if( kind == radical::Vector && optimized ) {

            ss << string_cpp(value);
            if(! var.empty() ) ss << ' ';
            ss << '*';

            if( ptr ) ss << '*';
            if(! var.empty() ) ss << var;
            return ss.str();
        }

        if( kind == radical::Vector ) {
            ss << "std::vector<" << string_cpp(value) << ">";
            if( ptr ) ss << '&';
            if(! var.empty() ) ss << ' ' << var;
            return ss.str();
        }

        if( kind == radical::Array && optimized ) {
            ss << string_cpp(value);

            if( var.empty() ) {
                if( ptr ) ss << "**";
            } else {
                ss << ' ';
                if( ptr ) ss << '*';
                ss << var << '[' << size << ']';
            }

            return ss.str();
        }

        if( kind == radical::Array ) {
            ss << "std::array<" << string_cpp(value) << ", " << size << ">";
            if( ptr ) ss << '&';
            if(! var.empty() ) ss << ' ' << var;
            return ss.str();
        }

        return "";
    }
};

struct function {
    std::string name;
    std::vector<type> argst;
    type ret;

    function(std::string name, std::vector<type> argst, type ret) : name(name), argst(argst), ret(ret) {}
};

struct desconstructor {
    enum reason { that };
    reason why;
    std::vector<std::string> identifiers;

    desconstructor(std::vector<std::string> identifiers) : identifiers(identifiers) {};
};

struct alias {
    std::string name;
    type data;

    alias(std::string name, type data) : name(name), data(data) {}
};


enum ParseResultKind {
    Function,
    Desconstructor,
    Alias
};

using ParseResult = std::tuple<
    ParseResultKind,
    std::variant<
        std::monostate,
        function,
        desconstructor,
        alias
    >
>;

using ParseResults = std::vector<ParseResult>;

template <typename T>
auto first(T tuple) {
    return std::get<0>(tuple);
}

template <typename T>
auto second(T tuple) {
    return std::get<1>(tuple);
}

std::tuple<bool, std::variant<std::monostate, type>> is_type(std::string& value) {
    bool ptr = false;
    std::string token = value;

    if( token[token.length()-1] == '*' ) {
        ptr = true;
        token = token.substr(0, token.length()-1);
    }

    for( auto type : types ) {
        if( token == std::get<0>(type) ) return { true, type::common(ptr, token) };
    }

    // for( std::string& alias : alias ) {
    //     if( token == alias ) return { true, type::common(ptr, token) };
    // }

    if( token[0] == '[' && token[token.length()-1] == ']' ) {

        if( token[1] == '*' && token[2] == ':' ) {
            std::string type = token.substr(3, token.length()-4);
            if( first(is_type(type)) ) return { true, type::vector(ptr, type) };
            else return { false, std::monostate() };
        }

        std::stringstream ss;
        int j = 1;
        for(; j < token.length(); j++ ) {
            if( isdigit(token[j]) ) ss << token[j];
            else if( token[j] == ':' ) break;
            else return { false, std::monostate() };
        }

        int size = std::atoi(ss.str().c_str());

        std::string type = token.substr(j+1, token.length()-j-2);
        if( first(is_type(type)) ) return { true, type::array(ptr, type, size) };
        else return { false, std::monostate() };
    }

    return { false, std::monostate() };
}

bool is_identifier(std::string& value) {
    if( first(is_type(value)) ) return false;
    std::regex r("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return std::regex_match(value, r);
}

ParseResults parse(CompilerParams& params, std::vector<std::string> tokens) {
    ParseResults results = {};

    for(int i = 0; i < tokens.size(); i++) {
        std::string token = tokens[i];
        std::string next = "";
        if( (i + 1) < tokens.size() ) next = tokens[i + 1];

        if( token[0] == '(' || token == "()" ) {
            i += 1;
            std::stringstream ss;
            ss << token;

            if( token != "()" ) for(; i < tokens.size(); i++) {
                ss << tokens[i];
                if( tokens[i][tokens[i].length()-1] == ')' ) break;
            }

            std::vector<std::string> identifiers;
            std::string nparen = ss.str().substr(1, ss.str().length()-2);

            ss.str("");
            for( int j = 0; j < nparen.length(); j++ ) {
                if( nparen[j] == ',' ) {
                    identifiers.push_back(ss.str());
                    ss.str("");
                } else {
                    ss << nparen[j];
                }
            }

            desconstructor d(identifiers);
            if( next == "@_" ) d.why = desconstructor::reason::that;
            i += 1;

            results.push_back({ ParseResultKind::Desconstructor, d });
            continue;
        }

        if( first(is_type(token)) ) {
            if( is_identifier(next)) {
                i += 2;

                std::vector<type> argst;
                for(; i < tokens.size(); i++) {
                    auto arg = is_type(tokens[i]);
                    if(! first(arg) && tokens[i] == "{" ) break;
                    else if(! first(arg) ) CompilerOutputs::Fatal("Syntax error - Function declaration");

                    auto second = std::get<1>(arg);
                    argst.push_back(std::get<type>(second));
                }

                auto second = std::get<1>(is_type(token));
                function func(next, argst, std::get<type>(second));
                results.push_back({ ParseResultKind::Function, func });

                continue;
            }
        }

        if( token == "alias" ) {
            auto err = [](){ CompilerOutputs::Fatal("Syntax error - a `alias` keyword. `alias id = type`"); };

            if( (! is_identifier(next)) || (i + 3) >= tokens.size()) err();
            auto sign = tokens[i + 2];
            if( sign != "=" ) err();

            auto rhs = tokens[i + 3];
            auto info = is_type(rhs);
            if(! first(info) ) err();
            i += 3;

            auto data = std::get<type>(second(info));
            alias a(next, data);
            results.push_back({ ParseResultKind::Alias, a });

            continue;
        }
    }

    return results;
}
