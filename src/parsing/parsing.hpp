#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../services/services.hpp"
#include "../tasks/tasks.hpp"

namespace devkit {

    void ParseServicesYml(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks
    );
}