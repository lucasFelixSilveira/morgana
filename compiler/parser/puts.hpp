#pragma once

#include <string>

struct puts_t {
    std::string str;
    std::size_t length;

    puts_t(std::string value, std::size_t length)
        : str(std::move(value)),
          length(length) {}
};
