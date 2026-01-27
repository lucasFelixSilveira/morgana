#pragma once

#include "../type.hpp"
#include <string>
#include <tuple>

struct allocation_data {
    std::string name;
    type data;
    allocation_data(std::string name, type data) : name(name), data(data) {}
};

using allocation = std::tuple<std::string , allocation_data>;
