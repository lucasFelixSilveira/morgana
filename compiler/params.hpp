// Isto armazena os dados do compilador
#pragma once

#include "sys.hpp"
#include <cstring>
#include <string>

typedef struct CompilerParams {
public:
    bool verbose;
    std::string cwd;
    std::string command;
    std::string main;
    std::string name;
    std::string target;
    bool optimized;

    static struct CompilerParams build(bool verbose, std::string cwd, std::string command, std::string main, std::string name, std::string target, bool optimized);
    static struct CompilerParams format(int argc, char **argv);
} CompilerParams;

CompilerParams
CompilerParams::build(bool verbose, std::string cwd, std::string command, std::string main, std::string name, std::string target, bool optimized)
{
    return (CompilerParams) { verbose, cwd, command, main, name, target, optimized };
}

CompilerParams
CompilerParams::format(int argc, char **argv)
{
    bool verbose = false;
    char *name = (char*) "project";
    char *cwd  = argv[0];
    char *command = argv[1];
    char *main = (char*) "main.morg";
    char *target = (char*) detectSystemInfo().arch.c_str();
    bool optimized = false;

    int i = 2;
    for(; i < argc; i++ ) {
        char *arg = argv[i];
        if( std::strcmp(argv[i], "-n") == 0 && (i + 1) < argc ) name = argv[++i];
        if( std::strcmp(argv[i], "-m") == 0 && (i + 1) < argc ) main = argv[++i];
        if( std::strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) target = argv[++i];
        if( std::strcmp(argv[i], "-O") == 0 ) optimized = true;
        if( std::strcmp(argv[i], "-v") == 0 ) verbose = true;
    }

    return CompilerParams::build(verbose, cwd, command, main, name, target, optimized);
}
