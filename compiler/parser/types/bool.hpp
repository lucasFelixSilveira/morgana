#pragma once

#include <regex>
#include <string>

struct morgana_bool {
    std::string regex;

    morgana_bool()
        : regex("false|true") {};

    bool check(std::string value) {
        return std::regex_match(value, std::regex(regex));
    }
};
