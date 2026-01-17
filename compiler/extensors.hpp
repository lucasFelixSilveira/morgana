#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cstring>
#include <ctime>
#include "compiler_outputs.hpp"
#include "runtime.hpp"

namespace fs = std::filesystem;

class GitManager {
private:
    static constexpr size_t BUFFER_SIZE = 128;

    static std::string getCurrentWorkspace() {
        try {
            return fs::current_path().string();
        } catch (const std::exception& e) {
            CompilerOutputs::Fatal("Unable to determine current workspace");
            return "";
        }
    }

    static std::string executeGitCommand(const std::string& command) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return "";
        }

        char buffer[BUFFER_SIZE];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }

        pclose(pipe);
        return result;
    }

    static bool validateRepository(const std::string& url) {
        std::string command = "git ls-remote " + url + " 2>&1";
        std::string output = executeGitCommand(command);

        if (output.empty()) {
            CompilerOutputs::Fatal("Unable to execute git command");
            return false;
        }

        if (output.find("fatal:") != std::string::npos ||
            output.find("not found") != std::string::npos ||
            output.find("Could not read from remote repository") != std::string::npos) {
            CompilerOutputs::Fatal("Repository not found or inaccessible");
            return false;
        }

        CompilerOutputs::Info("Repository validated successfully");
        return true;
    }

    static bool validateBranch(const std::string& url, const std::string& branch) {
        CompilerOutputs::Info("Validating branch: " + branch);

        std::string command = "git ls-remote --heads " + url + " " + branch + " 2>&1";
        std::string output = executeGitCommand(command);

        if (output.empty()) {
            CompilerOutputs::Fatal("Failed to validate branch");
            return false;
        }

        if (output.find("refs/heads/" + branch) == std::string::npos) {
            CompilerOutputs::Fatal("Branch '" + branch + "' does not exist");
            return false;
        }

        CompilerOutputs::Info("Branch validated successfully");
        return true;
    }

    static bool cloneRepository(const std::string& url, const std::string& branch, const std::string& tempDir) {
        try {
            if (fs::exists(tempDir)) {
                fs::remove_all(tempDir);
            }
            fs::create_directories(tempDir);
        } catch (const std::exception& e) {
            CompilerOutputs::Fatal("Failed to prepare temporary directory");
            return false;
        }

        std::string command = "git clone --depth 1 --branch " + branch + " " + url + " " + tempDir + " 2>&1";
        int result = std::system(command.c_str());

        if (result != 0) {
            CompilerOutputs::Fatal("Repository cloning failed");
            try {
                fs::remove_all(tempDir);
            } catch (...) {}
            return false;
        }

        CompilerOutputs::Info("Repository cloned successfully");
        return true;
    }

    static bool transferFiles(const std::string& source, const std::string& destination) {
        CompilerOutputs::Info("Transferring files to extensors directory");

        try {
            if (!fs::exists(destination)) {
                fs::create_directories(destination);
            }

            for (const auto& entry : fs::recursive_directory_iterator(source)) {
                const auto& sourcePath = entry.path();

                if (sourcePath.filename() == ".git") {
                    continue;
                }

                if (sourcePath.parent_path().filename() == ".git") {
                    continue;
                }

                auto relativePath = fs::relative(sourcePath, source);
                auto destinationPath = fs::path(destination) / relativePath;

                if (fs::is_directory(sourcePath)) {
                    fs::create_directories(destinationPath);
                } else {
                    if (sourcePath.extension() == ".lua") {
                        fs::copy(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
                    }
                }
            }

            CompilerOutputs::Info("File transfer completed");
            return true;
        } catch (const std::exception& e) {
            CompilerOutputs::Fatal("File transfer failed: " + std::string(e.what()));
            return false;
        }
    }

    static bool cleanupTemporaryDirectory(const std::string& path) {
        try {
            if (fs::exists(path)) {
                fs::remove_all(path);
                CompilerOutputs::Info("Temporary directory removed");
                return true;
            }
        } catch (const std::exception& e) {
            CompilerOutputs::Info("Unable to remove temporary directory");
            return false;
        }
        return true;
    }

    static bool isRepositoryURL(const std::string& input) {
        const std::vector<std::string> protocols = {
            "http://", "https://", "git://", "ssh://", "git@"
        };

        for (const auto& protocol : protocols) {
            if (input.find(protocol) == 0) {
                return true;
            }
        }
        return false;
    }

    static std::vector<std::string> extractRepositoryURLs(const std::string& workspace) {
        std::string targetFilePath = workspace + "/target.toml";
        if (!fs::exists(targetFilePath)) {
            CompilerOutputs::Fatal("target.toml not found in workspace");
            return {};
        }

        std::ifstream file(targetFilePath);
        if (!file.is_open()) {
            CompilerOutputs::Fatal("Unable to open target.toml");
            return {};
        }

        std::vector<std::string> repositories;
        std::string line;
        bool inExtensorsSection = false;

        while (std::getline(file, line)) {
            size_t commentStart = line.find('#');
            if (commentStart != std::string::npos) {
                line = line.substr(0, commentStart);
            }

            size_t start = line.find_first_not_of(" \t");
            size_t end = line.find_last_not_of(" \t");

            if (start == std::string::npos) continue;

            line = line.substr(start, end - start + 1);

            if (line == "[extensors_repository]") {
                inExtensorsSection = true;
                CompilerOutputs::Info("Located extensors repository section");
                continue;
            }

            if (inExtensorsSection && line.front() == '[') {
                break;
            }

            if (!inExtensorsSection) continue;

            if (line.find("extensors = [") != std::string::npos ||
                line.find(']') != std::string::npos) {
                continue;
            }

            if (line.empty()) continue;

            line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());
            line.erase(std::remove(line.begin(), line.end(), '\''), line.end());
            line.erase(std::remove(line.begin(), line.end(), ','), line.end());
            line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

            if (!line.empty() && isRepositoryURL(line)) {
                CompilerOutputs::Info("Discovered repository: " + line);
                repositories.push_back(line);
            }
        }

        file.close();

        if (repositories.empty()) {
            CompilerOutputs::Fatal("No repository URLs found in extensors section");
        }

        return repositories;
    }

    static std::string generateTemporaryDirectory(const std::string& workspace) {
        std::time_t timestamp = std::time(nullptr);
        return workspace + "/.extensor_temp_" + std::to_string(timestamp);
    }

public:
    static void processTargetConfiguration(const std::string& extensorName, const std::string& branch = "main") {
        std::string workspace = getCurrentWorkspace();
        if (workspace.empty()) {
            CompilerOutputs::Fatal("Unable to determine workspace");
            return;
        }

        std::vector<std::string> repositories = extractRepositoryURLs(workspace);
        if (repositories.empty()) {
            return;
        }

        CompilerOutputs::Info("Found " + std::to_string(repositories.size()) + " repositories");

        for (const auto& repositoryURL : repositories) {
            if (installExtensor(repositoryURL, extensorName, branch, workspace)) {
                return;
            }
        }

        CompilerOutputs::Fatal("Unable to install '" + extensorName + "' from any configured repository");
    }

private:
    static bool installExtensor(const std::string& repositoryURL, const std::string& extensorName,
                               const std::string& branch, const std::string& workspace) {

        if (!validateRepository(repositoryURL)) {
            return false;
        }

        if (!validateBranch(repositoryURL, branch)) {
            return false;
        }

        std::string temporaryDirectory = generateTemporaryDirectory(workspace);

        if (!cloneRepository(repositoryURL, branch, temporaryDirectory)) {
            return false;
        }

        std::string extensorFilePath = temporaryDirectory + DIR_SEP + extensorName + ".lua";
        if (!fs::exists(extensorFilePath)) {
            CompilerOutputs::Info("Extensor '" + extensorName + "' not found in repository");
            cleanupTemporaryDirectory(temporaryDirectory);
            return false;
        }

        std::string executablePath = Runtime::get_executable_path("morgana");
        size_t lastSeparator = executablePath.find_last_of("/\\");
        if (lastSeparator == std::string::npos) {
            CompilerOutputs::Fatal("Invalid executable path");
            cleanupTemporaryDirectory(temporaryDirectory);
            return false;
        }

        std::string extensorsDirectory = executablePath.substr(0, lastSeparator) +
                                         DIR_SEP + ".." + DIR_SEP + "extensors";

        if (!transferFiles(temporaryDirectory, extensorsDirectory)) {
            cleanupTemporaryDirectory(temporaryDirectory);
            return false;
        }

        if (!cleanupTemporaryDirectory(temporaryDirectory)) {
            return false;
        }

        CompilerOutputs::Log("Extensor '" + extensorName + "' installed successfully\n");
        return true;
    }
};

struct ExtensorManager {
public:
    static void install(char** arguments) {
        if (!arguments[2]) {
            CompilerOutputs::Fatal("Extensor name not provided");
            return;
        }

        std::string extensorName = arguments[2];
        CompilerOutputs::Info("Installing extensor: " + extensorName);

        std::string branch = "main";
        if (arguments[3]) {
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
