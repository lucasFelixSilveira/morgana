#pragma once

#include <string>

struct storage {
    std::string identifier;
    std::string value;

    storage() = default;

    storage(std::string identifier, std::string value)
        : identifier(identifier),
          value(value) {}
};
