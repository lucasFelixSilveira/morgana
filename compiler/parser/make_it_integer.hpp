#pragma once
#include <regex>
#include <string>

#define RET_KEYWORD 0
#define GPIO_INSTRUCTION 1
#define CALL_KEYWORD 2
#define TURN_KEYWORD 3
#define LOOP_KEYWORD 4
#define WAIT_KEYWORD 5
#define WAITMS_KEYWORD 6
#define READ_INSTRUCTION 7
#define ALLOC_INSTRUCTION 8
#define BRANCH_NOT_EQUAL_ZERO_KEYWORD 9
#define BRANCH_KEYWORD 10

#define DEFINE_LABEL 10000

int make_it_integer(const std::string& str) {

    std::regex r("^\\:\\:[a-zA-Z0-9_]+\\:\\:$");
    if( std::regex_match(str, r) ) return DEFINE_LABEL;

    if( str == "ret" ) return RET_KEYWORD;
    if( str == "call" ) return CALL_KEYWORD;
    if( str == "turn" ) return TURN_KEYWORD;
    if( str == "loop" ) return LOOP_KEYWORD;
    if( str == "wait" ) return WAIT_KEYWORD;
    if( str == "waitms" ) return WAITMS_KEYWORD;
    if( str == "brnez" ) return BRANCH_NOT_EQUAL_ZERO_KEYWORD;
    if( str == "br" ) return BRANCH_KEYWORD;

    if( str == "gpio" ) return GPIO_INSTRUCTION;
    if( str == "read" ) return READ_INSTRUCTION;
    if( str == "alloc" ) return ALLOC_INSTRUCTION;

    return -1;
}
