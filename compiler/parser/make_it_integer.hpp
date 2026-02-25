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
#define STORE_KEYWORD 11
#define LOAD_INSTRUCTION 12
#define ADD_INSTRUCTION 13
#define SUB_INSTRUCTION 14
#define DIV_INSTRUCTION 15
#define MUL_INSTRUCTION 16
#define BRANCH_EQUAL_ZERO_KEYWORD 17
#define ALERT_KEYWORD 18
#define BRANCH_GRANT_EQUAL_KEYWORD 19
#define BRANCH_LESS_EQUAL_KEYWORD 20
#define BRANCH_GRANT_KEYWORD 21
#define BRANCH_LESS_KEYWORD 22
#define TUPLE_INSTRUCTION 23
#define ADDINPTR_INSTRUCTION 24

#define DEFINE_LABEL 10000

int make_it_integer(const std::string& str) {

    std::regex r("^\\:\\:[a-zA-Z0-9_]+\\:\\:$");
    if( std::regex_match(str, r) ) return DEFINE_LABEL;

    if( str == "ret" ) return RET_KEYWORD;
    if( str == "call" ) return CALL_KEYWORD;

    if( str == "alert" ) return ALERT_KEYWORD;
    if( str == "loop" ) return LOOP_KEYWORD;

    if( str == "wait" ) return WAIT_KEYWORD;
    if( str == "waitms" ) return WAITMS_KEYWORD;

    if( str == "brnez" ) return BRANCH_NOT_EQUAL_ZERO_KEYWORD;
    if( str == "brez" ) return BRANCH_EQUAL_ZERO_KEYWORD;
    if( str == "brg" ) return BRANCH_GRANT_KEYWORD;
    if( str == "brl" ) return BRANCH_LESS_KEYWORD;
    if( str == "brge" ) return BRANCH_GRANT_EQUAL_KEYWORD;
    if( str == "brle" ) return BRANCH_LESS_EQUAL_KEYWORD;
    if( str == "br" ) return BRANCH_KEYWORD;

    if( str == "store" ) return STORE_KEYWORD;
    if( str == "alloc" ) return ALLOC_INSTRUCTION;
    if( str == "load" ) return LOAD_INSTRUCTION;

    if( str == "turn" ) return TURN_KEYWORD;
    if( str == "gpio" ) return GPIO_INSTRUCTION;
    if( str == "read" ) return READ_INSTRUCTION;

    if( str == "add" ) return ADD_INSTRUCTION;
    if( str == "sub" ) return SUB_INSTRUCTION;
    if( str == "div" ) return SUB_INSTRUCTION;
    if( str == "mul" ) return SUB_INSTRUCTION;

    if( str == "tuple" ) return TUPLE_INSTRUCTION;
    if( str == "addinptr" ) return ADDINPTR_INSTRUCTION;

    return -1;
}
