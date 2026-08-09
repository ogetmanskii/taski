#pragma once

#include <string>
#include <filesystem>
#include <sstream>

namespace devkit::FileUtils {
    std::string ReadFileUtf8(const std::filesystem::path& filePath) {
        if (!std::filesystem::exists(filePath)) {
            throw std::runtime_error("File does not exist: " + filePath.string());
        }

        if (!std::filesystem::is_regular_file(filePath)) {
            throw std::runtime_error("Path is not a regular file: " + filePath.string());
        }

        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        // Проверяем и удаляем BOM (Byte Order Mark), если он присутствует
        // BOM для UTF-8: 0xEF, 0xBB, 0xBF
        if (content.size() >= 3 &&
            static_cast<unsigned char>(content[0]) == 0xEF &&
            static_cast<unsigned char>(content[1]) == 0xBB &&
            static_cast<unsigned char>(content[2]) == 0xBF) {
            content.erase(0, 3);
        }

        return content;
    }
}