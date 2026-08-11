#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>

namespace devkit::Parser {

    std::unordered_map<std::string, std::string> ParseDotEnvFile(const std::filesystem::path& path);
}