#include "backend.hpp"
#include "compiler_outputs.hpp"
#include "params.hpp"
#include "mocks.hpp"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <sys/stat.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>

#include "extensors/manager.hpp"
#include "parser/parser.hpp"
#include "tokenizer.hpp"
#include "codegen.hpp"

CompilerParams params;

#ifdef _WIN32
std::string NIL_FD = " > NUL 2>&1 ";
# include <direct.h>
# define MKDIR(dir) _mkdir(dir)
#else
std::string NIL_FD = " > /dev/null 2>&1 ";
# include <unistd.h>
# define MKDIR(dir) mkdir(dir, 0700)
#endif

int
main(int argc, char **argv)
{
    auto start = std::chrono::high_resolution_clock::now();

    int min_arguments = 2;
    if( argc < min_arguments ) CompilerOutputs::Fatal("You need enter with a action. If you don't know the acceptable actions, use: help.");

    params = CompilerParams::format(argc, argv);
    if( params.command == "install" ) ExtensorManager::install(argv);
    if( params.command != "build" ) return 0;

    /* checks if the file is accessible */
    std::ifstream file(params.main, std::ios::binary | std::ios::ate);
    if(! file.is_open() ) CompilerOutputs::Fatal("Your main file is not valid. Try use -m to define the newest file");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> src(size);

    if(! file.read(src.data(), size) ) CompilerOutputs::Fatal("Your main file is not valid. Try use -m to define the newest file");

    /* Tokenization phase */
    std::vector<std::string> tokens = tokenize(src);
    std::cout << "lexer\n";

    /* Parser phase */
    ParseResults results = parse(tokens);
    #if MORGANA_DEBUG
        debug_print(results);
    #endif

    /* Code Generation phase */
    std::string generated = codegen(params, results);
    std::cout << "codegen\n";

    /* Create target directory if it doesn't exist */
    MKDIR((std::filesystem::current_path().string() + "/target").c_str());

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

    int success_code = 0;
    return success_code;
}
