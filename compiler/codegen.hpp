#pragma once

#include "compiler_outputs.hpp"
#include "libs/linux/include/lua.hpp"
#include "params.hpp"
#include "runtime.hpp"
#include <string>
#include <unistd.h>


static bool file_exists(const std::string& path) {
    return access(path.c_str(), R_OK) == 0;
}

static std::string find_lua_module(const std::string& dir, const std::string& modname) {
    std::string modpath = modname;
    std::replace(modpath.begin(), modpath.end(), '.', '/');

    std::vector<std::string> candidates = {
        dir + modname + ".lua",
        dir + modname + "/init.lua"
    };

    for( const auto& candidate : candidates )
    /* -> */ if( file_exists(candidate) ) return candidate;

    return "";
}

// Função C para morgana.require
static int morgana_require(lua_State* L) {
    const char* modname = luaL_checkstring(L, 1);

    lua_getfield(L, LUA_REGISTRYINDEX, "morgana_extensor_dir");
    const char* dir_cstr = lua_tostring(L, -1);
    std::string dir = dir_cstr ? dir_cstr : "./";
    lua_pop(L, 1);

    std::string module_file = find_lua_module(dir, modname);

    if( module_file.empty() ) {
        lua_pushnil(L);
        lua_pushfstring(L, "module '%s' not found in '%s'", modname, dir.c_str());
        return 2;
    }

    if( luaL_loadfile(L, module_file.c_str()) != LUA_OK ) return lua_error(L);

    lua_call(L, 0, 1);
    return 1;
}

std::string codegen(CompilerParams& params, ParseResults ast) {
    std::string path = Runtime::get_executable_path("morgana");
    auto [extensorExists, extensorPath] = Runtime::check_extensors(path, params.target);

    if(! extensorExists ) {
        CompilerOutputs::Fatal("No extensor found for target: " + params.target);
        return "";
    }

    CompilerOutputs::Info("Extensor found: " + extensorPath);
    CompilerOutputs::Info("Generating JSON to parse");

    std::string json = "{\"data\": " + Runtime::json(ast) + "}";

    lua_State *L = luaL_newstate();
    if (!L) {
        CompilerOutputs::Fatal("Failed to create Lua state");
        return "";
    }

    luaL_openlibs(L);

    std::string extensorDir = extensorPath;
    size_t lastSlash = extensorDir.find_last_of("/\\");
    if (lastSlash != std::string::npos) extensorDir = extensorDir.substr(0, lastSlash + 1);
    else extensorDir = "./";

    lua_newtable(L);

    lua_pushcfunction(L, morgana_require);
    lua_setfield(L, -2, "require");

    lua_setglobal(L, "morgana");

    lua_pushstring(L, extensorDir.c_str());
    lua_setfield(L, LUA_REGISTRYINDEX, "morgana_extensor_dir");

    if( luaL_loadfile(L, extensorPath.c_str()) != LUA_OK ) {
        CompilerOutputs::Fatal("Failed to load extensor: " + std::string(lua_tostring(L, -1)));
        lua_close(L);
        return "";
    }

    if( lua_pcall(L, 0, 0, 0) != LUA_OK ) {
        CompilerOutputs::Fatal("Error running extensor: " + std::string(lua_tostring(L, -1)));
        lua_close(L);
        return "";
    }

    lua_getglobal(L, "codegen");
    if(! lua_isfunction(L, -1) ) {
        CompilerOutputs::Fatal("Function 'codegen' not found in extensor");
        lua_close(L);
        return "";
    }

    lua_pushstring(L, json.c_str());

    if( lua_pcall(L, 1, 1, 0) != LUA_OK ) {
        CompilerOutputs::Fatal("Error in codegen: " + std::string(lua_tostring(L, -1)));
        lua_close(L);
        return "";
    }

    if(! lua_isstring(L, -1) ) {
        CompilerOutputs::Fatal("codegen must return a string");
        lua_close(L);
        return "";
    }

    std::string result = lua_tostring(L, -1);
    lua_close(L);

    CompilerOutputs::Info("Code generation completed");
    return result;
}
