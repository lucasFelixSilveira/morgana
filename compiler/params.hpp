// Isto armazena os dados do compilador
#pragma once

#include "sys.hpp"
#include <cstring>
#include <string>

#define MHz(x) ((x) * 1000000)

struct CompilerParams {
public:
    using hz = unsigned long long int;
    std::string cwd;
    std::string command;
    std::string main;
    std::string name;
    std::string target;
    std::string ffi_path;

    bool optimized;
    bool verbose;
    bool c_ffi;

    std::string mcu;
    std::string port;
    std::string programmer;
    hz frequency;

    CompilerParams(bool verbose, bool c_ffi, char *cwd,  char *command,  char *main,  char *name,  char *target, bool optimized, std::string mcu, hz frequency, std::string port, std::string programmer, char *ffi_path)
        : verbose(verbose),
          c_ffi(c_ffi),
          ffi_path(ffi_path),
          cwd(cwd),
          command(command),
          main(main),
          name(name),
          target(target),
          optimized(optimized),
          // MCUs
          mcu(mcu),
          frequency(frequency),
          port(port),
          programmer(programmer) {};

    CompilerParams() = default;

    static CompilerParams format(int argc, char **argv) {
        char *name = (char*) "project";
        char *cwd  = argv[0];
        char *command = argv[1];
        char *main = (char*) "main.morg";
        char *target = (char*) detectSystemInfo().arch.c_str();

        bool optimized = false;
        bool verbose = false;

        bool c_ffi = false;
        char *ffi_path = (char*) "internal.c";

        std::string os = detectSystemInfo().os;

        std::string programmer;
        std::string mcu;
        std::string port = (os == "Linux" ? "/dev/ttyUSB0" : os == "Darwin" ? "/dev/tty.usbmodem1411" : "COM1");
        hz frequency = -1;

        int i = 1;
        for(; i < argc; i++ ) {
            char *arg = argv[i];
            if( std::strcmp(argv[i], "-n") == 0 && (i + 1) < argc ) name = argv[++i];
            if( std::strcmp(argv[i], "-m") == 0 && (i + 1) < argc ) main = argv[++i];
            if( std::strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) target = argv[++i];
            if( std::strcmp(argv[i], "-cpath") == 0 && (i + 1) < argc ) ffi_path = argv[++i];
            if( std::strcmp(argv[i], "-O") == 0 ) optimized = true;
            if( std::strcmp(argv[i], "-v") == 0 ) verbose = true;
            if( std::strcmp(argv[i], "-ffi") == 0 ) c_ffi = true;

            if( std::strcmp(argv[i], "-MHz") == 0 && (i + 1) < argc ) frequency = MHz(std::stoul(argv[++i]));
            if( std::strcmp(argv[i], "-mcu") == 0 && (i + 1) < argc ) mcu = argv[++i];
            if( std::strcmp(argv[i], "-port") == 0 && (i + 1) < argc ) port = argv[++i];
            if( std::strcmp(argv[i], "-p") == 0 && (i + 1) < argc ) programmer = argv[++i];
        }

        if( std::string(target) == "avr" ) {
            if( mcu.empty() ) mcu = "atmega328p";
            if( frequency == -1 ) frequency = MHz(16);
            if( programmer.empty() ) programmer = "arduino";
        }

        return CompilerParams(verbose, c_ffi, cwd, command, main, name, target, optimized, mcu, frequency, port, programmer, ffi_path);
    }
};
