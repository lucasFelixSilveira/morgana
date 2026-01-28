#pragma once

#include <string>
#include <vector>

struct call {
    std::string func;
    std::vector<std::string> args;

    call(std::string func, std::vector<std::string> args)
        : func(func), args(args) {}
};
