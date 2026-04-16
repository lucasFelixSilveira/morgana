#pragma once

#include "compiler_outputs.hpp"
#include "params.hpp"
#include <fstream>
#include <sstream>

std::string link_mocks() {
    std::stringstream ss;

    // for( std::string mock : mocks ) {

    //     std::ifstream file("stl/" + mock + ".S", std::ios::binary | std::ios::ate);
    //     if(! file.is_open() ) CompilerOutputs::Fatal("Mock function file not found");

    //     std::streamsize size = file.tellg();
    //     file.seekg(0, std::ios::beg);

    //     std::vector<char> src(size);

    //     file.read(src.data(), size);
    //     file.close();

    //     ss << "\n" << src.data();
    // }

    return ss.str();
}
