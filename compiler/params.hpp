// Isto armazena os dados do compilador
#pragma once

#include "sys.hpp"
#include <cstring>
#include <string>

typedef struct CompilerParams {
public:
    std::string cwd;
    std::string command;
    std::string main;
    std::string target;
    bool optimized;

    static struct CompilerParams build(std::string cwd, std::string command, std::string main, std::string target, bool optimized);
    static struct CompilerParams format(int argc, char **argv);
} CompilerParams;

CompilerParams
CompilerParams::build(std::string cwd, std::string command, std::string main, std::string target, bool optimized)
{
    return (CompilerParams) { cwd, command, main, target, optimized };
}

CompilerParams
CompilerParams::format(int argc, char **argv)
{
    char *cwd  = argv[0];
    char *command = argv[1];
    char *main = (char*) "main.morg";
    char *target = (char*) detectSystemInfo().arch.c_str();
    bool optimized = false;

    int i = 2;
    for(; i < argc; i++ ) {
        char *arg = argv[i];
        if( std::strcmp(argv[i], "-m") == 0 && (i + 1) < argc ) main = argv[++i];
        if( std::strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) target = argv[++i];
        if( std::strcmp(argv[i], "-O") == 0 ) optimized = true;
    }

    return CompilerParams::build(cwd, command, main, target, optimized);
}
