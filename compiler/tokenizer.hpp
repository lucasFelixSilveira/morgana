#pragma once

#include <string>
#include <vector>
std::vector<std::string> tokenize(std::vector<char> src) {
    const std::string delimiters = " \t\n\r";

    std::vector<std::string> tokens;
    std::string token;
    for (char c : src) {
        if (delimiters.find(c) != std::string::npos) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}
