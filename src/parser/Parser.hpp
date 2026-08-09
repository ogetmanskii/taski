#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace devkit {
    class Service;
    class Task;
}

namespace devkit::Parser {

    using namespace devkit;

    std::unordered_map<std::string, std::string> ParseDotEnvFile(const std::string& filename);

    void ParseEnvironmentYamlFile(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks
    );
}