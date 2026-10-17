#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace devkit::Parser {

void ParseDotEnvFile(const std::filesystem::path& path, std::unordered_map<std::string, std::string>& outEnv);
}