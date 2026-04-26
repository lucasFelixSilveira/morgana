#pragma once

#include <string>
#include <vector>

std::vector<std::string> tokenize(std::vector<char> src) {
    const std::string delimiters = " \t\n\r";

    std::vector<std::string> tokens;
    std::string token;
    bool in_string = false;

    for( size_t i = 0; i < src.size(); i++ ) {
        char c = src[i];

        if( c == '\r' ) continue;

        if( in_string ) {
            token += c;

            if (c == '"') {
                tokens.push_back(token);
                token.clear();
                in_string = false;
            }

            continue;
        }

        if( c == '"' ) {
            if(! token.empty() ) {
                tokens.push_back(token);
                token.clear();
            }

            token += c;
            in_string = true;
            continue;
        }

        if( delimiters.find(c) != std::string::npos ) {
            if(! token.empty() ) {
                tokens.push_back(token);
                token.clear();
            }
        } else token += c;
    }

    if(! token.empty() ) {
        tokens.push_back(token);
    }

    return tokens;
}
