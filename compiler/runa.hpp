#pragma once

#include "libs/linux/include/runa.hpp"
#include <iostream>
#include <string>

int amount;
void reset_fields() { amount = 0; }
void add_fields(int count) { amount += count; }
void add_field() { amount++; }

void print(Runa *runa);

void runa_needs(Runa *runa, runa_callback write, runa_callback writeln, runa_callback tabs) {
    runa_push_function(runa, (char*) "print", (runa_callback)print, 1);
    runa_push_function(runa, (char*) "write", (runa_callback)write, 1);
    runa_push_function(runa, (char*) "writeln", (runa_callback)writeln, 1);
    runa_push_function(runa, (char*) "tabs", (runa_callback)tabs, 1);
}

void cap(Runa *runa) { runa_push_table(runa, (char*) "node", amount); }

void print(Runa *runa) {
    RunaValueFFI arg = runa_peek_arg(runa, 0);
    char *str = runa_value_to_string(runa, arg);
    std::cout << std::string(str) << std::endl;
    runa_optional(RUNA_FREE_STRING_BY_VALUE, runa_str_free, str, arg);
    runa_value_free(arg);
}
