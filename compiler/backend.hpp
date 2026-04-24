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

// Versão melhorada - mais segura e eficiente
#ifdef _WIN32
#include <windows.h>
#define CAND(x, y)                                                                                     \
    ([]() -> bool {                                                                                    \
        char* path = std::getenv("PATH");                                                              \
        if(! path ) return false;                                                                      \
        std::string path_str = path;                                                                   \
        size_t start = 0, end;                                                                         \
        while((end = path_str.find(';', start)) != std::string::npos) {                                \
            std::string dir = path_str.substr(start, end - start);                                     \
            std::string full_path = dir + "\\" + #x + ".exe";                                          \
            DWORD attrs = GetFileAttributesA(full_path.c_str());                                       \
            if( attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY) ) return true; \
            start = end + 1;                                                                           \
        }                                                                                              \
        return false;                                                                                  \
    }()) && (y)

#else
#include <unistd.h>
#include <sys/stat.h>
#define CAND(x, y)                                                                                     \
    ([]() -> bool {                                                                                    \
        std::string cmd = "command -v " + std::string(#x);                                             \
        return std::system((cmd + " > /dev/null 2>&1").c_str()) == 0;                                  \
    }()) && (y)
#endif

const bool end = true;

struct Backend {
    static void assemble(CompilerParams& params, std::string s, std::string o, std::string exe) {
        std::stringstream breaker;
        breaker << "\n" << Colorizer::DARK_GREY << "└─ " << Colorizer::RESET;

        auto const LINUX = 0, WINDOWS = 1, MACOS = 2;
        auto sys = detectSystemInfo();
        auto checkout = sys.os == "Linux" ? LINUX : (sys.os == "Darwin" ? MACOS : WINDOWS);

        switch (checkout) {
            case LINUX: {
                /* Compile from x86_64-linux without cross-compilation.
                 *
                 * 1. Use 'as' to compile the Morgana IR assembly output to an object file.
                 * 2. Use 'gcc' to compile the C FFI source to an object file (if enabled).
                 * 3. Use 'ld' to link the object files together into an executable.
                 */
                if( sys.arch == "x86_64" && params.target == "x86_64-linux" ) {
                    std::string as = "as \"" + s + "\" -o \"" + o + "\" > /dev/null 2>&1";
                    if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile Morgana IR to object file using as");

                    if( params.c_ffi ) {
                        std::string gcc = "gcc -nostartfiles -c \"" + params.ffi_path + "\" -o \"" + o + ".ffi\"" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
                        if( std::system(gcc.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile C FFI to object file using gcc");

                        std::string ld = "ld \"" + o + "\" \"" + o + ".ffi\" -lc --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o \"" + exe + "\"" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
                        if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link C FFI to Morgana IR object file using ld");
                        return;
                    }

                    std::string ld = "ld \"" + o + "\" -o \"" + exe + "\" > /dev/null 2>&1";
                    if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link Morgana IR to object file using ld");
                    return;
                };

                /* Cross-compilation for x86_64-windows using MinGW-w64 toolchain.
                 *
                 * 1. Use 'as' to assemble Morgana IR output to COFF object file.
                 * 2. Use 'objcopy' to fix symbol underscores.
                 * 3. Use 'gcc' as linker driver to produce final executable.
                 * 4. If C FFI enabled, compile C source and link together.
                 */
                if( sys.arch == "x86_64" && params.target == "x86_64-windows" ) {
                    bool mingw = CAND("x86_64-w64-mingw32-gcc", CAND("x86_64-w64-mingw32-as", CAND("x86_64-w64-mingw32-objcopy", end)));
                    if(! mingw ) CompilerOutputs::Fatal("Failed to find Mingw-w64 toolchain. Install it and try again." + breaker.str() + "https://www.mingw-w64.org/downloads/");

                    std::string as = "x86_64-w64-mingw32-as --64 \"" + s + "\" -o \"" + o + ".old\"" + (params.verbose ? "" : " > /dev/null 2>&1");
                    if( std::system(as.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to assemble to COFF");

                    std::string objcopy = "x86_64-w64-mingw32-objcopy \"" + o + ".old\" \"" + o + "\"" + (params.verbose ? "" : " > /dev/null 2>&1");
                    if( std::system(objcopy.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to convert object file");

                    std::string libpath = "/usr/x86_64-w64-mingw32/lib";
                    FILE* find = popen("find /usr -name \"libmsvcrt.a\" 2>/dev/null | grep \"x86_64-w64-mingw32\" | head -1", "r");
                    if( find ) {
                        char buf[256];
                        if( fgets(buf, sizeof(buf), find) ) {
                            std::string path(buf);
                            path.erase(path.find_last_not_of(" \n\r\t") + 1);
                            size_t pos = path.find_last_of('/');
                            if( pos != std::string::npos ) libpath = path.substr(0, pos);
                        }
                        pclose(find);
                    }

                    std::string link_flags = "-L\"" + libpath + "\" -lmsvcrt -lkernel32 -lmingw32 -lmingwex -Wl,--image-base,0x140000000";

                    if( params.c_ffi ) {
                        std::string gcc = "x86_64-w64-mingw32-gcc -m64 -c \"" + params.ffi_path + "\" -o \"" + o + ".ffi\"" + (params.verbose ? "" : " > /dev/null 2>&1");
                        if( std::system(gcc.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile C FFI");

                        std::string ld = "x86_64-w64-mingw32-gcc -m64 \"" + o + ".ffi\" \"" + o + "\" -o \"" + exe + "\" " + link_flags + " -Wl,--subsystem,windows -Wl,--entry,WinMain";
                        if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link executable");

                        return;
                    }

                    std::string ld = "x86_64-w64-mingw32-gcc -m64 \"" + o + "\" -o \"" + exe + "\" " + link_flags + " -Wl,--subsystem,windows -Wl,--entry,WinMain";
                    if( std::system(ld.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link executable");
                    return;
                }
            } break;
        }

        if( params.target == "xtensa" ) {
            bool xtensa = CAND("idf.py", end);
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

        if( params.target == "avr" ) {
            bool avr = CAND("avr-gcc", CAND("avr-objcopy", CAND("avrdude", CAND("avr-size", end))));
            if(! avr ) CompilerOutputs::Fatal("Failed to find avr toolchain. Install it and try again." + breaker.str() + " https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers");

            std::string build = "avr-gcc -mmcu=" + params.mcu + " -Os -DF_CPU=" + std::to_string(params.frequency) + " -Os -c target/output.s -o target/output.o" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
            if( std::system(build.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to build avr");

            std::string linking = "avr-gcc -mmcu=" + params.mcu + " -Os -o target/output target/output.o" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
            if( std::system(linking.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to link avr");

            std::string copy = "avr-objcopy -O ihex target/output target/output.hex" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
            if( std::system(copy.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to copy avr");

            if( params.verbose ) {
                std::string verify = "avr-size --format=avr --mcu=" + params.mcu + " target/output";
                if( std::system(verify.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to verify avr");
            }

            std::string flash = "avrdude -c " + params.programmer + " -p " + params.mcu + " -P " + params.port + " -b 115200 -U flash:w:target/output.hex:i" + std::string(params.verbose ? "" : " > /dev/null 2>&1");
            if( std::system(flash.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to flash avr");
            return;
        }

    }
};
