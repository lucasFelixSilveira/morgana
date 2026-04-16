#pragma once

#include "base.hpp"
#include <cmath>
#include <cstdint>
#include <regex>

struct morgana_integer : morgana_type {
    int bits;
    bool unsigned_;
    std::string regex_;

    morgana_integer(bool unsigned_, int bits)
        : bits(bits),
          unsigned_(unsigned_),
          regex_( unsigned_ ? "[0-9]+" : "[-]?[0-9]+" ) {};

    int matrix() const {
        return std::log2(bits / 8)-2;
    }

    bool check(std::string asciz) {
        if( bits <= 0 || bits > 64 ) return false;
        if(! std::regex_match(asciz, std::regex(regex_)) ) return false;

        int64_t value = std::stoull(asciz);

        if( unsigned_ ) {
            if (value < 0) return false;

            uint64_t max = (bits == 64) ? UINT64_MAX : ((1ULL << bits) - 1);
            return (uint64_t)value <= max;
        }

        int64_t max = (bits == 64) ? INT64_MAX : ((1LL << (bits - 1)) - 1);
        int64_t min = (bits == 64) ? INT64_MIN : -(1LL << (bits - 1));

        return value >= min && value <= max;
    }
};
