#pragma once

#include "base.hpp"
#include <regex>

struct morgana_ptr : morgana_type {
    std::string regex_;

    morgana_ptr()
        : regex_( "(ptr|(.*\\*))" ) {};

    bool check(std::string asciz) {
        return std::regex_match(asciz, std::regex(regex_));
    }
};
