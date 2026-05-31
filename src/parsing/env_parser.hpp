#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

class EnvParser {

public:
    inline std::string TrimWhitespace(const std::string& str) const {
        std::size_t first = str.find_first_not_of(" \t");
        std::size_t last = str.find_last_not_of(" \t");
        if (first == std::string::npos) {
            return "";
        }
        return str.substr(first, last - first + 1);
    }

    std::unordered_map<std::string, std::string> ParseFromFile(const std::string& filename);
};