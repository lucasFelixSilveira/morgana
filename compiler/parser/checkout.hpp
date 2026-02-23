#pragma once

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <variant>

#include "type.hpp"

template <typename T>
bool contains(std::vector<T> vec, T value) {
    return std::find(std::begin(vec), std::end(vec), value) != std::end(vec);
}

template <typename T>
auto first(T tuple) {
    return std::get<0>(tuple);
}

template <typename T>
auto second(T tuple) {
    return std::get<1>(tuple);
}

template <typename T>
auto third(T tuple) {
    return std::get<2>(tuple);
}

std::tuple<std::string, int> types[] = {
    { "u8", 1 },   { "i8", 1 },
    { "u16", 2 }, { "i16", 2 },
    { "u32", 4 }, { "i32", 4 },
    { "u64", 8 }, { "i64", 8 },
    { "void", 0 }
};

std::tuple<bool, std::variant<std::monostate, type>> is_type(std::string& value) {
    bool ptr = false;
    std::string token = value;

    if( value == "ptr" ) return { true, type::common(true, "i8") };

    if( token[token.length()-1] == '*' ) {
        ptr = true;
        token = token.substr(0, token.length()-1);
    }

    for( auto type : types )
    /* -> */ if( token == std::get<0>(type) ) return { true, type::common(ptr, token) };


    // for( std::string& alias : alias ) {
    //     if( token == alias ) return { true, type::common(ptr, token) };
    // }

    if( token[0] == '[' && token[token.length()-1] == ']' ) {

        // if( token[1] == '*' && token[2] == ':' ) {
            // std::string type = token.substr(3, token.length()-4);
            // if( first(is_type(type)) ) return { true, type::vector(ptr, type) };
            // else return { false, std::monostate() };
        // }

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

bool is_identifier(std::string value) {
    if( first(is_type(value)) ) return false;
    std::stringstream ss;
    const char *c = value.c_str();
    int i = 0; while( c[i] != '(' && c[i] != 0 ) ss << c[i++];

    std::regex r("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return std::regex_match(ss.str(), r);
}

bool is_number(std::string& value) {
    std::regex r("^[0-9]+$");
    return std::regex_match(value, r);
}
