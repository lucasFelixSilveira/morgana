#pragma once

#include "compiler_outputs.hpp"
#include "libs/linux/include/lua.hpp"
#include "params.hpp"
#include "extensors/runtime.hpp"
#include "parser.hpp"
#include "parser/comp.hpp"
#include "parser/symbols.hpp"
#include "parser/types/integer.hpp"
#include "parser/types/strong_alias.hpp"
#include <sstream>
#include <string>
#include <unistd.h>
#include <variant>
#include <vector>
#include <memory>

struct ASTIterator {
    ParseResults ast;
    size_t current_index;
};

enum LUAT {
    LUA_INTEGER_STRING,
    LUA_TEXT_STRING,
};

#define JSON_ENCODER(ss, vec, constructor)                                    \
    {                                                                         \
        auto build = [&](auto& iter) -> std::string constructor;              \
        ss << "{ \"data\": [";                                                \
        bool first = true;                                                    \
        for( auto iter : vec ) {                                              \
            ss << ((first) ? "" : ", ") << build(iter) << "";                 \
            if( first ) first = !first;                                       \
        };                                                                    \
        ss << "] }";                                                          \
    }

std::stack<std::shared_ptr<ASTIterator>> iterators;
std::shared_ptr<ASTIterator> current_iterator = nullptr;

static void morgana_push_ctx(std::vector<std::string> ctx) {
    if( current_iterator ) {
        auto saved_iterator = std::make_shared<ASTIterator>();
        saved_iterator->ast = current_iterator->ast;
        saved_iterator->current_index = current_iterator->current_index;
        iterators.push(saved_iterator);
    }

    auto new_iterator = std::make_shared<ASTIterator>();
    new_iterator->ast = parse(ctx);
    new_iterator->current_index = 0;
    current_iterator = new_iterator;
}

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
        if( file_exists(candidate) ) return candidate;

    return "";
}

static void push_ast_node_to_lua(lua_State* L, ParseResult& node) {
    lua_newtable(L);

    lua_pushinteger(L, first(node));
    lua_setfield(L, -2, "kind");

    switch(first(node)) {
        case ParseResultKind::Function: {
            auto data = std::get<function>(second(node));
            std::stringstream ss;

            // append the current iterator on the stack
            // and then put the function iterator on the
            // current_iterator global variable
            morgana_push_ctx(data.body);

            // store the function name on the table
            // for lua know all the IDs of the function
            lua_pushstring(L, data.name.c_str());
            lua_setfield(L, -2, "name");

            // store the function parameters type
            // using JSON encoder
            JSON_ENCODER(ss, data.argst, {
                if( std::holds_alternative<morgana_strong_alias>(iter) ) {
                    auto data = std::get<0>(std::get<morgana_strong_alias>(iter));
                    if( std::holds_alternative<morgana_integer>(data) ) {
                        auto x = std::get<morgana_integer>(data);
                        iter = symbol(x);
                    }
                }

                if( std::holds_alternative<morgana_integer>(iter) ) return std::get<morgana_integer>(iter).json();
                return std::string();
            });
            lua_pushstring(L, ss.str().c_str());
            lua_setfield(L, -2, "params");

            ss.str("");
            ss.clear();
        } break;

        case ParseResultKind::Loop: {
            auto data = std::get<loop>(second(node));

            // append the current iterator on the stack
            // and then put the loop iterator on the
            // current_iterator global variable
            morgana_push_ctx(data.body);
        } break;

        case ParseResultKind::BranchNotEqualZero: {
            auto data = std::get<brnez>(second(node));

            lua_pushstring(L, first(data).c_str());
            lua_setfield(L, -2, "identifier");

            lua_pushstring(L, second(data).c_str());
            lua_setfield(L, -2, "label");
        } break;

        case ParseResultKind::Branch: {
            auto data = std::get<std::string>(second(node));

            lua_pushstring(L, data.c_str());
            lua_setfield(L, -2, "label");
        } break;

        case ParseResultKind::Label: {
            auto data = std::get<std::string>(second(node));

            lua_pushstring(L, data.c_str());
            lua_setfield(L, -2, "identifier");
        } break;

        case ParseResultKind::Wait:
        case ParseResultKind::WaitMS: {
            auto data = std::get<int>(second(node));

            // calculate the delay in milliseconds
            // considering the base unit is seconds
            // for the `wait` keyword.
            const int second = 1000;
            int base = ParseResultKind::Wait == first(node);
            int mul = base * second + !base;
            lua_pushinteger(L, data * mul);
            lua_setfield(L, -2, "ms");
        } break;

        case ParseResultKind::Turn: {
            auto data = std::get<turn>(second(node));

            lua_pushinteger(L, data.pin);
            lua_setfield(L, -2, "pin");

            lua_pushboolean(L, data.toggle);
            lua_setfield(L, -2, "toggle");
        } break;

        case ParseResultKind::Read: {
            auto data = std::get<declaration<mcu_read>>(second(node));

            lua_pushstring(L, first(data).c_str());
            lua_setfield(L, -2, "identifier");

            lua_pushinteger(L, first(second(data)));
            lua_setfield(L, -2, "pin");

            lua_pushboolean(L, second(second(data)));
            lua_setfield(L, -2, "digital");
        } break;

        default: break;
    }
}

static int morgana_next(lua_State* L) {
    if(! current_iterator || current_iterator->ast.empty() ) {
        lua_pushnil(L);
        lua_pushstring(L, "No AST available or empty AST");
        return 2;
    }

    if( current_iterator->current_index >= current_iterator->ast.size() ) {
        if (iterators.empty()) {
            lua_pushnil(L);
            lua_pushstring(L, "End of AST");
            return 2;
        }

        current_iterator = iterators.top();
        iterators.pop();

        if(! current_iterator || current_iterator->ast.empty() ) {
            lua_pushnil(L);
            lua_pushstring(L, "No AST after restoration");
            return 2;
        }

        if( current_iterator->current_index >= current_iterator->ast.size() ) {
            lua_pushnil(L);
            lua_pushstring(L, "Restored iterator also finished");
            return 2;
        }
    }

    auto node = current_iterator->ast.at(current_iterator->current_index);
    current_iterator->current_index++;

    push_ast_node_to_lua(L, node);
    return 1;
}

static int morgana_reset(lua_State* L) {
    lua_pushboolean(L, true);
    return 1;
}

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
    CompilerOutputs::Info("Generating code via extensor");

    while (!iterators.empty()) iterators.pop();

    current_iterator = std::make_shared<ASTIterator>();
    current_iterator->ast = ast;
    current_iterator->current_index = 0;

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

    // Adiciona morgana.require
    lua_pushcfunction(L, morgana_require);
    lua_setfield(L, -2, "require");

    // Adiciona morgana.next
    lua_pushcfunction(L, morgana_next);
    lua_setfield(L, -2, "next");

    // Adiciona morgana.reset
    lua_pushcfunction(L, morgana_reset);
    lua_setfield(L, -2, "reset");

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

    lua_pushnil(L);

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

    // Limpar após uso
    current_iterator.reset();
    while(! iterators.empty() ) iterators.pop();

    CompilerOutputs::Info("Code generation completed");
    return result;
}
