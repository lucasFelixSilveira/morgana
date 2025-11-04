#include "compiler_outputs.hpp"
#include "params.hpp"
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

#include "parser.hpp"
#include "tokenizer.hpp"
#include "codegen.hpp"

#ifdef _WIN32
# include <direct.h>
# define MKDIR(dir) _mkdir(dir)
#else
# include <unistd.h>
# define MKDIR(dir) mkdir(dir, 0700)
#endif

int
main(int argc, char **argv)
{
    auto start = std::chrono::high_resolution_clock::now();

    int min_arguments = 2;
    if( argc < min_arguments ) CompilerOutputs::Fatal("You need enter with a action. If you don't know the acceptable actions, use: help.");

    CompilerParams params = CompilerParams::format(argc, argv);
    if( params.command != "build" ) return 0;

    CompilerOutputs::Log("Starting the compilation!");

    /* checks if the file is accessible */
    std::ifstream file(params.main, std::ios::binary | std::ios::ate);
    if(! file.is_open() ) CompilerOutputs::Fatal("Your main file is not valid. Try use -m to define the newest file");

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> src(size);

    if(! file.read(src.data(), size) ) CompilerOutputs::Fatal("Your main file is not valid. Try use -m to define the newest file");

    /* Tokenization phase */
    std::vector<std::string> tokens = tokenize(src);

    /* Parser phase */
    ParseResults results = parse(params, tokens);

    /* Code Generation phase */
    std::string generated = codegen(params, results);

    /* Create target directory if it doesn't exist */
    struct stat st = {0};
    std::string createFolder = "mkdir target > /dev/null 2>&1";
    if( stat("target", &st) == -1 && std::system(createFolder.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to create target directory");

    /* Write CPP to target/output.cpp */
    std::ofstream outFile("target/output.cpp");
    if(! outFile.is_open() ) {
        CompilerOutputs::Fatal("Failed to open output file target/output.cpp");
    }
    outFile << generated;
    outFile.close();
    if( outFile.fail() ) {
        CompilerOutputs::Fatal("Failed to write Morgana IR to target/output.cpp");
    }

    std::filesystem::path absPath = std::filesystem::absolute("target/output.cpp");

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
              << Colorizer::DARK_GREY << "|" << Colorizer::BOLD_YELLOW << " (not compiled yet)";

    /* Compile Morgana CPP to object file using g++ silently */
    std::string gpp = "g++ " + absPath.string() + " > /dev/null 2>&1";
    if( std::system(gpp.c_str()) != 0 ) CompilerOutputs::Fatal("Failed to compile Morgana IR to object file using g++");

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
              << Colorizer::DARK_GREY << "(.exe)" << std::endl;

    int success_code = 0;
    return success_code;
}
