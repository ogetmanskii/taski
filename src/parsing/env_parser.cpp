#include "env_parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include "inja/inja.hpp"
#include "../parsing/inja_env.hpp"

std::unordered_map<std::string, std::string> EnvParser::ParseFromFile(const std::string& filename) {
    static inja::Environment injaEnv = devkit::GetInjaEnvironment();

    std::unordered_map<std::string, std::string> data;

    std::ifstream file(filename);
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