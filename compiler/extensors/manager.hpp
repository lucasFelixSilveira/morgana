#pragma once

#include <string>

#include "../compiler_outputs.hpp"
#include "git.hpp"

struct ExtensorManager {
    static void install(char** arguments) {
        if(! arguments[2] ) {
            CompilerOutputs::Fatal("Extensor name not provided");
            return;
        }

        std::string extensorName = arguments[2];
        CompilerOutputs::Info("Installing extensor: " + extensorName);

        std::string branch = "main";
        if( arguments[3] ) {
            branch = arguments[3];
            CompilerOutputs::Info("Using branch: " + branch);
        }

        try {
            GitManager::processTargetConfiguration(extensorName, branch);
        } catch (const std::exception& e) {
            CompilerOutputs::Fatal("Configuration processing failed: " + std::string(e.what()));
        }

        std::exit(0);
    }
};
