#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <tuple>

#include "codegen.hpp"

namespace fs = std::filesystem;

struct Runtime {
public:

    static std::string get_executable_path(const std::string& command_name) {
        try {
            std::string path_env = std::getenv("PATH") ? std::getenv("PATH") : "";
            std::stringstream ss(path_env);
            std::string path;

            while(std::getline(ss, path, ':')) {
                fs::path exe_path = fs::path(path) / command_name;
                if( fs::exists(exe_path) && (fs::status(exe_path).permissions() & fs::perms::owner_exec) != fs::perms::none )
                /* -> */ return exe_path.string();
            }
        } catch (...) {}

        exit(-1);
    }


    #ifdef _WIN32
    #define DIR_SEP '\\'
    #else
    #define DIR_SEP '/'
    #endif

    static std::tuple<bool, std::string> check_extensors(std::string base_path, std::string extensor ) {
        size_t last_sep = base_path.find_last_of("/\\");
        if( last_sep == std::string::npos ) {
            return { false, base_path };
        }

        std::string full_path =
            base_path.substr(0, last_sep) +
            std::string(1, DIR_SEP) + ".." +
            std::string(1, DIR_SEP) + "extensors" +
            std::string(1, DIR_SEP) + extensor + ".lua";

        std::ifstream file(full_path);
        bool exists = file.is_open();
        if( file.is_open() ) file.close();

        return { exists, full_path };
    }

    static std::string json(ParseResults& ast) {

    }
};
