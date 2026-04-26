#pragma once

#include <stdio.h>
#include <string>

size_t interpret_string(std::string str) {
    size_t len = 0;
    bool scaped = false;
    for( char c : str ) {
        bool changed = false;
        if( scaped ) { scaped = false; changed = true; }
        if( c == '\\' && changed == false ) { scaped = true; continue; }
        len++;
    }
    return len;
}
