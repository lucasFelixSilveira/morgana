#pragma once

#include <cmath>
#include <cstdlib>
#include <regex>
#include <string>

struct morgana_integer {
    bool is_signed = false;
    std::string regex;
    int bits = 0;

    morgana_integer(int bits, bool is_signed)
        : bits(bits),
          is_signed(is_signed),
          regex(is_signed ? "[-]?\\d+" : "\\d+") {}

    bool check(std::string value) {
        if(!std::regex_match(value, std::regex(regex))) return false;

        long long num = std::atoll(value.c_str());

        if(! is_signed ) {
            unsigned long long max = (1ULL << bits) - 1;
            if( num < 0 || (unsigned long long) num > max ) return false;
        }

        long long min = -(1LL << (bits - 1));
        long long max =  (1LL << (bits - 1)) - 1;

        if( num < min || num > max ) return false;

        return true;
    }


    int matrixPos() const {
        return std::log2(bits) - 2;
    }

    std::string json() {
        std::stringstream ss;
        ss << "{ ";
        ss << " \"bytes\": " << ( bits / 8 ) << ", ";
        ss << " \"matrix\": " << ( matrixPos() + 1 ) << ", ";
        ss << " \"ptr\": false";
        ss << "}";
        return ss.str();
    }
};
