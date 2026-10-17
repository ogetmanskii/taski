#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "inja/environment.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "util/TemplateUtils.hpp"

using namespace devkit;

namespace fs = std::filesystem;

namespace {
std::string TrimWhitespace(const std::string& str) {
  std::size_t first = str.find_first_not_of(" \t");
  std::size_t last = str.find_last_not_of(" \t");
  if (first == std::string::npos) {
    return "";
  }
  return str.substr(first, last - first + 1);
}
}  // namespace

namespace devkit::Parser {

void ParseDotEnvFile(const fs::path& path, std::unordered_map<std::string, std::string>& outEnv) {
  static inja::Environment injaEnv = GetInjaEnvironment();

  std::ifstream file(path.string());
  if (!file.is_open()) {
    return;
  }

  nlohmann::json jsonData;
  std::string line;
  while (std::getline(file, line)) {
    // skip empty lines and comments
    if (line.empty() || line[0] == '#' || line[0] == ';') {
      continue;
    }

    // parse key-value pairs
    auto delimiterPos = line.find('=');
    if (delimiterPos == std::string::npos) {
      throw std::runtime_error("Invalid line format: " + line);
    }
    std::string key = TrimWhitespace(line.substr(0, delimiterPos));
    std::string value = TrimWhitespace(line.substr(delimiterPos + 1));

    outEnv[key] = injaEnv.render(value, jsonData);
    jsonData[key] = outEnv[key];
  }
}
}  // namespace devkit::Parser