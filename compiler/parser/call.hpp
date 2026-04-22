#pragma once

#include <string>
#include <vector>
struct call_t {
    std::string identifier;
    std::vector<std::string> args;

    call_t() = default;
    call_t(std::string id, std::vector<std::string> vec)
        : identifier(id),
          args(vec) {};
};
