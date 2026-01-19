#pragma once

#include <vector>
#include <string>

#include "../toml/reader.hpp"
#include "git.hpp"

std::vector<std::string> extractRepositoryURLs(const std::string& workspace) {
    TOMLReader reader("morgana", "target.toml");
    TOMLReader::Values data = reader.get({"extensors", "repositories"});

    if( std::holds_alternative<TOMLReader::Error>(data) ) {
        auto err = std::get<TOMLReader::Error>(data);
        CompilerOutputs::Fatal("Failed to read extensors section: " + std::get<0>(err));
    }

    CompilerOutputs::Info("Located extensors section");
    std::vector<std::string> repositories = reader.check<std::vector<std::string>>("repositories", data);

    for( const auto& url : repositories ) {
        if(! url.empty() && GitManager::isRepositoryURL(url) ) {
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
