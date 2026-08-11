#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>

namespace devkit::Parser {

    void ParseDotEnvFile(const std::filesystem::path& path, std::unordered_map<std::string, std::string>& outEnv);
}