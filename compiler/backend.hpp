#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <sys/stat.h>
#include <vector>

#include "compiler_outputs.hpp"
#include "params.hpp"

struct Backend {
    static void assemble(CompilerParams& params, std::string s, std::string o, std::string exe) {
        std::stringstream breaker;
        breaker << "\n" << Colorizer::DARK_GREY << "└─ " << Colorizer::RESET;

        // Compile with AS
        if( params.target == "x86_64" ) {
            std::string as = "as \"" + s + "\" -o \"" + o + "\" > /dev/null 2>&1";
            if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile Morgana IR to object file using as");

            std::string ld = "ld \"" + o + "\" -o \"" + exe + "\" > /dev/null 2>&1";
            if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link Morgana IR to object file using ld");

            return;
        }

        if( params.target == "xtensa" ) {
            bool xtensa = false;
            #ifdef _WIN32
                xtensa = std::system("where idf.py >nul 2>nul") == 0;
            #else
                xtensa = std::system("which idf.py > /dev/null 2>&1") == 0;
            #endif

            if(! xtensa ) CompilerOutputs::Fatal("Failed to find xtensa toolchain (idf.py). Install it and try again." + breaker.str() + " https://docs.espressif.com/projects/esp-idf/");

            std::filesystem::path absPath = std::filesystem::absolute("target/xtensa");

            struct stat st = {0};
            std::string createFolder = "mkdir target/xtensa > /dev/null 2>&1";
            if( stat("target/xtensa", &st) == -1 && std::system(createFolder.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to create xtensa directory");

            // make root configs
            std::stringstream ss;
            ss << "cmake_minimum_required(VERSION 3.16)\n"
               << "include($ENV{IDF_PATH}/tools/cmake/project.cmake)\n"
               << "project(" << params.name << ")";

            std::FILE* file = std::fopen("target/xtensa/CMakeLists.txt", "w+");
            if(! file ) CompilerOutputs::Fatal("Failed to create CMakeLists.txt");
            std::fprintf(file, "%s\n", ss.str().c_str());
            std::fclose(file);

            ss.str("");
            ss.clear();

            // make main folder configs
            createFolder = "mkdir target/xtensa/main > /dev/null 2>&1";
            if( stat("target/xtensa/main", &st) == -1 && std::system(createFolder.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to create xtensa main package");

            file = std::fopen("target/xtensa/main/CMakeLists.txt", "w+");
            if(! file ) CompilerOutputs::Fatal("Failed to create CMakeLists.txt");
            std::fprintf(file, "idf_component_register( SRCS \"m.S\" )");
            std::fclose(file);

            // Copy output.s to main folder
            std::ifstream output(s, std::ios::binary | std::ios::ate);
            std::streamsize size = output.tellg();
            output.seekg(0, std::ios::beg);
            std::vector<char> src(size);
            if(! output.read(src.data(), size) ) CompilerOutputs::Fatal("Failed to read output file");

            file = std::fopen("target/xtensa/main/m.S", "w+"); // usage .S to pass before for the C pre-compiler
            if(! file ) CompilerOutputs::Fatal("Failed to create m.S");
            for( char c : src ) std::putc(c, file);
            std::fclose(file);

            // Build xtensa with idf.py
            std::string build = "cd target/xtensa && idf.py build flash" + std::string(params.verbose ? "" : " > /dev/null 2>&1") + " && cd ../../";
            if( std::system(build.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to build xtensa");
            return;
        }

    }
};
