#pragma once
#include <string>
#include <vector>

#include "symbols.hpp"
#include "type.hpp"

struct function {
    std::string name;
    std::vector<symbol> argst;
    type ret;
    std::vector<std::string> body;

    function(std::string name, std::vector<symbol> argst, type ret, std::vector<std::string> body) : name(name), argst(argst), ret(ret), body(body) {}
};
