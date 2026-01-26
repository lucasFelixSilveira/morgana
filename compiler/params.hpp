// Isto armazena os dados do compilador
#pragma once

#include "sys.hpp"
#include <cstring>
#include <string>
#include <variant>

#define MHz(x) ((x) * 1000000)

struct CompilerParams {
public:
    using hz = unsigned long long int;
    bool verbose;
    std::string cwd;
    std::string command;
    std::string main;
    std::string name;
    std::string target;
    bool optimized;

    std::string mcu;
    hz frequency;
    std::string port;
    std::string programmer;

    CompilerParams(bool verbose, std::string cwd, std::string command, std::string main, std::string name, std::string target, bool optimized, std::string mcu, hz frequency, std::string port, std::string programmer)
        : verbose(verbose),
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

    static CompilerParams format(int argc, char **argv) {
        bool verbose = false;
        char *name = (char*) "project";
        char *cwd  = argv[0];
        char *command = argv[1];
        char *main = (char*) "main.morg";
        char *target = (char*) detectSystemInfo().arch.c_str();
        bool optimized = false;

        char *programmer = nullptr;
        char *mcu = nullptr;
        char *port = nullptr;
        hz frequency = -1;

        std::string os = detectSystemInfo().os;

        int i = 2;
        for(; i < argc; i++ ) {
            char *arg = argv[i];
            if( std::strcmp(argv[i], "-n") == 0 && (i + 1) < argc ) name = argv[++i];
            if( std::strcmp(argv[i], "-m") == 0 && (i + 1) < argc ) main = argv[++i];
            if( std::strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) target = argv[++i];
            if( std::strcmp(argv[i], "-O") == 0 ) optimized = true;
            if( std::strcmp(argv[i], "-v") == 0 ) verbose = true;

            if( std::strcmp(argv[i], "-MHz") == 0 && (i + 1) < argc ) frequency = MHz(std::stoul(argv[++i]));
            if( std::strcmp(argv[i], "-mcu") == 0 && (i + 1) < argc ) mcu = argv[++i];
            if( std::strcmp(argv[i], "-port") == 0 && (i + 1) < argc ) port = argv[++i];
            if( std::strcmp(argv[i], "-p") == 0 && (i + 1) < argc ) programmer = argv[++i];
        }

        if( std::string(target) == "avr" ) {
            if( mcu == nullptr ) mcu = (char*) "atmega328p";
            if( frequency == -1 ) frequency = MHz(16);

            if( programmer == nullptr ) programmer = (char*) "arduino";
            if( port == nullptr ) port = (char*) (os == "Linux" ? "/dev/ttyUSB0" : os == "Darwin" ? "/dev/tty.usbmodem1411" : "COM1");
        }

        return CompilerParams(verbose, cwd, command, main, name, target, optimized, mcu, frequency, port, programmer);
    }
};
