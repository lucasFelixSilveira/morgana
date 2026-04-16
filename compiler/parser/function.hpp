#pragma once

#include "symbols.hpp"
#include <string>
#include <vector>

struct function {
    std::string name;
    std::vector<symbol> args;
    std::vector<std::string> body;
    bool noframe;

    function(std::string name, std::vector<symbol> args, std::vector<std::string> body, bool noframe)
        : name(name), args(args), body(body), noframe(noframe) {}

    function() = default;
};
