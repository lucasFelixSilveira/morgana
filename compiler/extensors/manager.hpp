#pragma once

#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "../compiler_outputs.hpp"
#include "../libs/eva.hpp"
#include "runtime.hpp"

struct ExtensorManager {
    static void install(char** arguments) {
        if(! arguments[2] ) {
            CompilerOutputs::Fatal("Extensor name not provided");
            return;
        }

        std::string extensor_name = arguments[2];

        std::string branch = "main";
        if( arguments[3] ) branch = arguments[3];

        std::vector<std::string> repositories;
        auto eva_path = std::filesystem::current_path() / "target.eva";
        if(! std::filesystem::exists(eva_path) ) repositories = { "https://github.com/Carla-Corp/extensors.git" };
        else try { eva driver(eva_path.string());
                   auto r = driver.get<eva::list>("extensors", "repositories");
                   if(! r.first ) goto _continue;
                   for( size_t i = 0; i < r.second.size(); i++ )
                   /* -> */ try { repositories.push_back(eva::data(r.second.operator[]<std::string>(i)));
                            } catch(...) {}

        } catch(...) { repositories = { "https://github.com/Carla-Corp/extensors.git" }; }

        _continue: ;

        std::string morgana = Runtime::get_executable_path("morgana");
        for( std::string repository : repositories ) {
            std::string clone = "git clone " + repository + " --branch " + branch + " --depth 1";
            std::system(clone.c_str());

            auto rgx = std::regex("(.*)[/]([a-zA-Z0-9_-]+)(.git)?$");
            std::smatch matches;
            if(! std::regex_search(repository, matches, rgx) ) continue;
            std::string name = matches[2];
            auto folder = std::filesystem::current_path() / name;

            if(! std::filesystem::exists(folder / (extensor_name + ".lua")) ) {
                std::filesystem::remove_all(folder);
                continue;
            };

            std::cout << Colorizer::BOLD_GREEN << "The extensor " << extensor_name << " was found in " << name << ".\n" << Colorizer::RESET;
            std::cout << "Do you want to use it? (Y/n): ";

            char choice;
            std::cin.get(choice);
            bool yes_choice = (choice == 'y' || choice == 'Y' || choice == '\n' || choice == '\r');
            if(! yes_choice ) continue;

            auto dest = std::filesystem::path(morgana).parent_path() / "../extensors" / (extensor_name + ".lua");
            std::filesystem::remove(dest);
            std::filesystem::copy(folder / (extensor_name + ".lua"), dest);

            CompilerOutputs::Log("The extensor " + extensor_name + " was installed successfully.\n");
            std::filesystem::remove_all(folder);
            break;
        }

        std::exit(0);
    }
};
