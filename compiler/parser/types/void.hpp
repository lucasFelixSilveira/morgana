#pragma once

#include "base.hpp"

struct morgana_void : morgana_type {
    morgana_void() = default;

    bool check(std::string asciz) {
        return asciz == "void";
    }
};
