#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <tuple>

#include "../compiler_outputs.hpp"

namespace fs = std::filesystem;

struct Runtime {
public:

    static std::string get_executable_path(const std::string& command_name) {
        try {
            const char* path_env = std::getenv("PATH");
            if (path_env) {
                std::string path_env_str = path_env;
                std::stringstream ss(path_env_str);
                std::string path;

                #ifdef _WIN32
                char separator = ';';
                std::string suffix = ".exe";
                #else
                char separator = ':';
                std::string suffix = "";
                #endif

                while (std::getline(ss, path, separator)) {
                    if (path.empty()) continue;

                    fs::path full_path;

                    #ifdef _WIN32
                    if( path.front() == '"' && path.back() == '"' ) {
                        path = path.substr(1, path.length() - 2);
                    }
                    full_path = fs::path(path) / (command_name + suffix);
                    #else
                    full_path = fs::path(path) / (command_name + suffix);
                    #endif

                    std::string full_path_str = full_path.string();

                    if (fs::exists(full_path_str)) {
                        #ifdef _WIN32
                        return full_path_str;
                        #else
                        auto perms = fs::status(full_path_str).permissions();
                        if( (perms & fs::perms::owner_exec) != fs::perms::none ||
                            (perms & fs::perms::group_exec) != fs::perms::none ||
                            (perms & fs::perms::others_exec) != fs::perms::none
                        ) return full_path_str;
                        #endif
                    }
                }
            }

            // Se não encontrou no PATH, procura no diretório atual
            std::string local_path;
            #ifdef _WIN32
            local_path = ".\\" + command_name + ".exe";
            #else
            local_path = "./" + command_name;
            #endif

            if( fs::exists(local_path) ) {
                return fs::absolute(local_path).string();
            }

            #ifdef _WIN32
            local_path = ".\\" + command_name;
            if( fs::exists(local_path) ) return fs::absolute(local_path).string();
            #endif

        } catch (const std::exception& e) {
            CompilerOutputs::Fatal("Error finding executable: " + std::string(e.what()));
        }

        CompilerOutputs::Fatal("Executable not found in PATH or current directory: " + command_name);
        return "";
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
};
