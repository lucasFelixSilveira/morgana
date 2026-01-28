#pragma once
#include <string>
#include <vector>

#include "type.hpp"

struct function {
    std::string name;
    std::vector<type> argst;
    type ret;
    std::vector<std::string> body;

    function(std::string name, std::vector<type> argst, type ret, std::vector<std::string> body) : name(name), argst(argst), ret(ret), body(body) {}
};
