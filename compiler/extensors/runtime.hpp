#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <tuple>
#include <variant>

#include "../parser.hpp"

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
        std::stringstream ss;
        ss << "[";
        bool ffirst = true;
        for(auto& [key, value] : ast) {
            ss << ((ffirst) ? "" : ", ") << "{";
            if( ffirst ) ffirst = !ffirst;

            ss << "\"kind\": " << key;
            switch(key) {

                case ParseResultKind::Function: {
                    auto data = std::get<function>(value);
                    auto body = parse(data.body);

                    ss << ", ";
                    ss << "\"name\": \"" << data.name << "\",";
                    ss << "\"params\": [";

                    bool first = true;
                    for( auto param : data.argst ) {
                        ss << ((first) ? "" : ", ") << param.json() << "";
                        if( first ) first = !first;
                    };

                    ss << "],";
                    ss << "\"body\": " << json(body);
                } break;

                case ParseResultKind::Desconstructor: {
                    auto data = std::get<desconstructor>(value);

                    ss << ", ";
                    ss << "\"why\": " << data.why << ",";
                    ss << "\"id\": [";

                    bool first = true;
                    for( auto param : data.identifiers ) {
                        ss << ((first) ? "" : ", ") << "{\"string\":\"" << param << "\"}";
                        if( first ) first = !first;
                    }

                    ss << "]";
                } break;

                case ParseResultKind::Load: {
                    auto data = std::get<std::string>(value);
                    ss << ", ";
                    ss << "\"what\": \"" << data << "\"";
                } break;

                case ParseResultKind::VectorAllocation: {
                    auto data = std::get<std::tuple<type, std::vector<int>>>(value);

                    std::stringstream ss1;
                    for( auto i : std::get<1>(data) ) ss1 << ((ss1.tellp() == 0) ? "" : ", ") << "{ \"value\": " << i << " }";

                    ss << ", ";
                    ss << "\"type\": " << std::get<0>(data).json() << ",";
                    ss << "\"values\": [" << ss1.str() << "]";
                } break;

                case ParseResultKind::Store: {
                    auto data = std::get<store>(value);

                    ss << ", ";
                    ss << "\"src\": \"" << data.value << "\",";
                    ss << "\"dest\": \"" << data.identifier << "\"";
                } break;

                case ParseResultKind::Allocation: {
                    auto data = std::get<allocation>(value);

                    ss << ", ";
                    ss << "\"type\": " << data.data.json() << ",";
                    ss << "\"name\": \"" << data.name << "\"";
                } break;

                case ParseResultKind::GetPointerElement: {
                    auto data = std::get<std::tuple<std::string, std::string>>(value);

                    ss << ", ";
                    ss << "\"src\": \"" << std::get<0>(data) << "\",";
                    ss << "\"index\": \"" << std::get<1>(data) << "\"";
                } break;

                default: break;
            }
            ss << "}";
        }
        ss << "]";
        return ss.str();
    }
};
