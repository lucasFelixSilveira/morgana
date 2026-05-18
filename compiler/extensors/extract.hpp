#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "../compiler_outputs.hpp"
#include "../libs/eva.hpp"

static bool isRepositoryURL(const std::string& input) {
    const std::vector<std::string> protocols = {
        "http://", "https://", "git://", "ssh://", "git@"
    };

    for( const auto& protocol : protocols )
    /* -> */ if (input.find(protocol) == 0) return true;
    return false;
}

std::vector<std::string> extractRepositoryURLs(const std::string& workspace) {
    eva reader(std::filesystem::current_path().string() + "/target.toml");
    auto [exist, data] = reader.get<eva::list>("extensors", "repositories");
    if( exist ) CompilerOutputs::Fatal("Failed to read the '@extensors' namespace and 'repositories' field");

    CompilerOutputs::Info("Located extensors section");

    std::vector<std::string> repositories;
    for( int i = 0; i < data.size(); i++ ) {
        try { auto url = eva::data(data.operator[]<std::string>(i));
              if( url.empty() ) continue;
              if(! isRepositoryURL(url) ) {
                CompilerOutputs::Info("\nSkipping invalid repository URL: " + url);
                continue;
              }
              repositories.push_back(url);
        } catch(std::runtime_error e)
        { CompilerOutputs::Fatal("\nFailed to extract repository URL at index " + std::to_string(i) + ": " + e.what()); }
    }

    if( repositories.empty() ) {
        CompilerOutputs::Fatal("No valid repository URLs found in extensors section");
    }

    return repositories;
}
