#pragma once

#include <string>
#include <vector>

struct call {
    std::string func;
    std::vector<std::string> args;
    bool expect_result;

    call(std::string func, std::vector<std::string> args, bool expect_result)
        : func(func), args(args), expect_result(expect_result) {}
};
