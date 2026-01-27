#pragma once
#include <string>
#include <vector>

#include "type.hpp"

struct function {
    std::string name;
    std::vector<type> argst;
    type ret;
    std::vector<std::string> body;

    function(std::string name, std::vector<type> argst, type ret, std::vector<std::string> body) : name(name), argst(argst), ret(ret), body(body) {}
};

#define FUNCTION \
if( first(is_type(token)) ) { \
    if( is_identifier(next)) { \
        i += 2; \
 \
        std::vector<type> argst; \
        for(; i < tokens.size(); i++ ) { \
            auto arg = is_type(tokens[i]); \
            if(! first(arg) && tokens[i] == "{" ) break; \
            else if(! first(arg) ) CompilerOutputs::Fatal("Syntax error - Function declaration"); \
 \
            auto second = std::get<1>(arg); \
            argst.push_back(std::get<type>(second)); \
        } \
 \
        auto second = std::get<1>(is_type(token)); \
 \
        std::vector<std::string> body = {}; \
        int j = i + 1; \
        for(; j < tokens.size(); j++ ) { \
            auto token = tokens[j]; \
            if( token == "}" ) break; \
            body.push_back(token); \
        } \
 \
        i = j; \
        function f(next, argst, std::get<type>(second), body); \
        results.push_back({ ParseResultKind::Function, f }); \
 \
        continue; \
    } \
}
