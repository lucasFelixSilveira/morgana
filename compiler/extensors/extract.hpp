#pragma once

#include <filesystem>
#include <vector>
#include <string>

#include "../toml/reader.hpp"

static bool isRepositoryURL(const std::string& input) {
    const std::vector<std::string> protocols = {
        "http://", "https://", "git://", "ssh://", "git@"
    };

    for( const auto& protocol : protocols )
    /* -> */ if (input.find(protocol) == 0) return true;
    return false;
}
std::vector<std::string> extractRepositoryURLs(const std::string& workspace) {
    TOMLReader reader("morgana", std::filesystem::relative("target.toml").string());
    TOMLReader::Values data = reader.get({"extensors", "repositories"});

    if( std::holds_alternative<TOMLReader::Error>(data) ) {
        auto err = std::get<TOMLReader::Error>(data);
        CompilerOutputs::Fatal("Failed to read extensors section: " + std::get<0>(err));
    }

    CompilerOutputs::Info("Located extensors section");
    std::vector<std::string> repositories = reader.check<std::vector<std::string>>("repositories", data);

    for( const auto& url : repositories ) {
        if(! url.empty() && isRepositoryURL(url) ) {
            CompilerOutputs::Info("Discovered repository: " + url);
            repositories.push_back(url);
        }
        else if(! url.empty() ) CompilerOutputs::Info("Skipping invalid repository URL: " + url);
    }

    if( repositories.empty() ) {
        CompilerOutputs::Fatal("No valid repository URLs found in extensors section");
    }

    return repositories;
}
