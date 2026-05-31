#pragma once

#include "backend.hpp"
#include "codegen.hpp"
#include "common.hpp"
#include "extensors/manager.hpp"
#include "mocks.hpp"
#include "params.hpp"
#include "compiler_outputs.hpp"
#include "parser/parser.hpp"
#include "tokenizer.hpp"

#include <iomanip>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#ifdef _WIN32
std::string NIL_FD = " > NUL 2>&1 ";
#else
std::string NIL_FD = " > /dev/null 2>&1 ";
#endif


#define COMMANDS_FIELDS \
    X(version, "use to view the version of the compiler") \
    X(help, "use to list all available commands") \
    X(build, "use to build the project as binary") \
    X(install, "use to install a new extensor")

struct Commands {
    int status = 0;

    static bool help(CompilerParams& params) {
        std::cout << "\e[1;34mAvailable commands\e[0m:\n";
        constexpr int padding = 8;
        #define X(cmd, desc) \
        std::cout << "  \e[1;33m" << std::left << std::setw(padding) << #cmd << "\e[0m - \e[1;30m" << desc << "\e[0m\n";
        COMMANDS_FIELDS
        #undef X
        return 0;
    }

    static bool version(CompilerParams& params) {
        std::cout << "carla version " << MORGANA_VERSION << " (" << MORGANA_PS << ")\n";
        return 0;
    }

    static bool build(CompilerParams& params);
    static bool install(CompilerParams& params);

    Commands(CompilerParams& params) {
        #define X(cmd, desc) if( params.command == #cmd ) status = cmd(params);
        COMMANDS_FIELDS
        #undef X
    }
};


bool Commands::install(CompilerParams& params) {
    ExtensorManager::install(params.argv);
    return true;
}

bool Commands::build(CompilerParams& params) {
    auto start = std::chrono::high_resolution_clock::now();

    /* checks if the file is accessible */
    std::ifstream file(params.main, std::ios::binary);
    if(! file.is_open() ) CompilerOutputs::Fatal("Your main file is not valid. Try use -m to define the newest file");

    std::string content;
    std::getline(file, content, '\0');

    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

    std::vector<char> src(content.begin(), content.end());

    /* Tokenization phase */
    std::vector<std::string> tokens = tokenize(src);

    ParseResults results = parse(tokens);
    #if MORGANA_DEBUG
        debug_print(results);
    #endif

    /* Code Generation phase */
    std::string generated = codegen(params, results);

    /* Create target directory if it doesn't exist */
    auto target = std::filesystem::current_path() / "/target";
    std::filesystem::create_directory(target);

    /* Write CPP to target/output.cpp */
    std::ofstream outFile("target/output.s");
    if(! outFile.is_open() ) {
        CompilerOutputs::Fatal("Failed to open output file target/output.s");
    }
    outFile << generated << link_mocks();
    outFile.close();
    if( outFile.fail() ) {
        CompilerOutputs::Fatal("Failed to write Morgana IR to target/output.s");
    }

    std::filesystem::path absPath = std::filesystem::absolute("target/output.s");
    std::filesystem::path absPathObj = std::filesystem::absolute("target/output.o");
    std::filesystem::path absPathExe = std::filesystem::absolute("target/output");

    /* calculate time of the **INTERNAL** compilation process */
    auto mid = std::chrono::high_resolution_clock::now();
    auto midMS = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);

    float midSeconds = midMS.count() / 1000000.0;
    std::stringstream duration;
    duration << "Total " << Colorizer::BOLD_RED << "Morgana" << Colorizer::RESET
             << " compilation proccess time: " << Colorizer::BOLD_GREEN
             << std::fixed << std::setprecision(2) << midSeconds << "s"
             << Colorizer::RESET << "\n";
    CompilerOutputs::Log(duration.str());

    std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::RESET << "Morgana Object generated "
              << Colorizer::DARK_GREY << "|" << Colorizer::BOLD_YELLOW << " (not compiled yet)" << Colorizer::RESET ;

    /* Compile Morgana Assembly to object file using the right assemler silently */
    Backend::assemble(params, absPath.string(), absPathObj.string(), absPathExe.string());

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    float seconds = ms.count() / 1000000.0;

    CompilerOutputs::ClearCurrentLine();
    duration.str("");
    duration << "Total " << Colorizer::BOLD_RED << "Morgana" << Colorizer::RESET
             << " + " << Colorizer::BOLD_BLUE << "Extern compilation process" << Colorizer::RESET
             << " compilation proccess time: " << Colorizer::BOLD_YELLOW
             << std::fixed << std::setprecision(2) << seconds << "s"
             << Colorizer::RESET << "\n";
    CompilerOutputs::Log(duration.str());
    std::cout << Colorizer::DARK_GREY << "└─ " << Colorizer::RESET << "Object emitted "
              << Colorizer::DARK_GREY << "|" << Colorizer::BOLD_YELLOW << " ./target/output "
              << Colorizer::DARK_GREY << "(.exe)" << Colorizer::RESET << std::endl;

    return true;
}
