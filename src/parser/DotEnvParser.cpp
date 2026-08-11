#include <unordered_map>
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>
#include <util/StringUtils.hpp>
#include <util/TemplateUtils.hpp>

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
}

namespace devkit::Parser {

    std::unordered_map<std::string, std::string> ParseDotEnvFile(const fs::path& path) {
        static inja::Environment injaEnv = GetInjaEnvironment();

        std::unordered_map<std::string, std::string> data;

        std::ifstream file(path.string());
        if (!file.is_open()) {
            return data;
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

            data[key] = injaEnv.render(value, jsonData);
            jsonData[key] = data[key];
        }
        return data;
    }
}