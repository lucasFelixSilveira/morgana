#pragma once

#include <iostream>
#include <linux/limits.h>
#include <string>

#include "compiler_outputs.hpp"
#include "libs/linux/include/lua.hpp"

#include "params.hpp"
#include "parser.hpp"
#include "runtime.hpp"

#if defined(__linux__) || defined(__APPLE__)
#define UNIX_LIKE
#endif

std::string codegen(CompilerParams& params, ParseResults ast) {
    std::string path = Runtime::get_executable_path("morgana");
    auto [extensorExists, extensorPath] = Runtime::check_extensors(path, params.target);

    if(! extensorExists ) {
        CompilerOutputs::Fatal("No extensor found for target: " + params.target);
        return "";
    }

    CompilerOutputs::Info("Extensor found: " + extensorPath);

    lua_State *L = luaL_newstate();
    if (!L) {
        CompilerOutputs::Fatal("Failed to create Lua state");
        return "";
    }

    luaL_openlibs(L);

    int r = luaL_dofile(L, extensorPath.c_str());
    if( r != LUA_OK ) {
        CompilerOutputs::Fatal("Lua error: " + std::string(lua_tostring(L, -1)));
        lua_close(L);
        return "";
    }

    if(! lua_isstring(L, -1) ) {
        CompilerOutputs::Fatal("Lua extensor did not return a string");
        lua_close(L);
        return "";
    }

    std::string generatedCode = lua_tostring(L, -1);
    lua_close(L);

    CompilerOutputs::Info("Code generation completed");
    return generatedCode;
}
