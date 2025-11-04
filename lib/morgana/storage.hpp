#pragma once

#include <memory>
#include <tuple>
#include <vector>

struct Storage {
public:
    long long addr = 0;
    std::vector<std::tuple<long long, std::string>> aliases;

    std::string string();
};
