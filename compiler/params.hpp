#pragma once

#include "libs/eva.hpp"
#include "sys.hpp"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#define MHz(x) ((x) * 1000000)

struct CompilerParams {
public:
    char **argv;

    using hz = unsigned long long int;
    std::string cwd, command, main, name, target, output;
    bool verbose;

    CompilerParams() = default;

    static CompilerParams format(int argc, char **argv) {
        std::string cwd  = argv[0];
        std::string command = argv[1];
        std::string main = "main.morg";
        std::string name = "project";
        std::string target = detectSystemInfo().arch + "-" + detectSystemInfo().os;
        std::string output = "";
        bool verbose = false;

        std::string eva_path = (std::filesystem::current_path() / "target.eva").string();
        if( std::filesystem::exists(eva_path) ) {
            eva driver(eva_path);
            try {
                auto n = driver.get<std::string>("target", "name");
                if( n.first ) name = n.second;

                auto o = driver.get<eva::map>("target", "output");
                if( o.first ) {
                    auto f = o.second.operator[]<std::string>("format");
                    if( f.first ) output = (
                        f.second == "native-bin" || f.second == "shared-object" || f.second == "static-archive"
                        ? f.second
                        : "native-bin"
                    );
                }
            } catch(...) {}
        }

        int i = 1;
        for(; i < argc; i++ ) {
            char *arg = argv[i];
            if( std::strcmp(argv[i], "-m") == 0 && (i + 1) < argc ) main = std::string(argv[++i]);
            if( std::strcmp(argv[i], "-o") == 0 && (i + 1) < argc ) target = std::string(argv[++i]);
            if( std::strcmp(argv[i], "-v") == 0 ) verbose = true;
        }

        CompilerParams params{};

        params.command = command;
        params.output = output;
        params.target = target;
        params.main = main;
        params.argv = argv;

        return params;
    }
};
