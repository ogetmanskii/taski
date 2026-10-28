#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace devkit {

class Service;
class Task;

}  // namespace devkit

namespace devkit::Parser {

void ParseEnvironmentYmlFile(const std::filesystem::path& filePath,
                             const std::unordered_map<std::string, std::string>& env,
                             std::vector<std::unique_ptr<Service>>& outServices,
                             std::vector<std::unique_ptr<Task>>& outTasks);

}