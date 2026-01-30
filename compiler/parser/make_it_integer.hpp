#pragma once
#include <string>

#define RET_KEYWORD 0
#define GPIO_INSTRUCTION 1
#define CALL_KEYWORD 2
#define TURN_KEYWORD 3

int make_it_integer(const std::string& str) {
    if( str == "ret" ) return RET_KEYWORD;
    if( str == "call" ) return CALL_KEYWORD;
    if( str == "turn" ) return TURN_KEYWORD;

    if( str == "gpio" ) return GPIO_INSTRUCTION;

    return -1;
}
