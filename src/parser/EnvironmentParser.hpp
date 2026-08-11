#pragma once

#include <filesystem>
#include <unordered_map>
#include <string>
#include <vector>

using namespace devkit;

namespace devkit {
    class Service;
    class Task;
}

namespace devkit::Parser {

    void ParseEnvironmentYmlFile(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks
    );
}