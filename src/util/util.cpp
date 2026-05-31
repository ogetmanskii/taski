#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <iostream>
#include <atomic>
#include <algorithm>
#include "../logging/console_logging.hpp"
#include "../processes/processes.hpp"

namespace devkit {

    std::string TrimToSingleLine(const std::string& string) {
        std::istringstream stream(string);
        std::string line;
        std::string result;

        while (std::getline(stream, line)) {
            // Убираем \r в конце строки (Windows формат)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Обрезаем пробелы в начале и конце строки
            auto start = std::find_if_not(line.begin(), line.end(),
                [](unsigned char c) { return std::isspace(c); });
            auto end = std::find_if_not(line.rbegin(), line.rend(),
                [](unsigned char c) { return std::isspace(c); }).base();

            if (start < end) {
                if (!result.empty()) {
                    result += ' ';
                }
                result.append(start, end);
            }
        }

        return result;
    }

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

    void WaitForNoActiveProcess(const std::string& exePath, const std::string& argsPattern) {
        if (argsPattern.empty()) {
            info("-- Waiting for process to terminate: {}", exePath);
        } else {
            info("-- Waiting for process to terminate: {}\n   with args: {}", exePath, argsPattern);
        }
        auto wExePath = StringToWString(exePath);
        auto wArgsPattern = StringToWString(argsPattern);
        while (ProcessExists(wExePath, wArgsPattern)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void WaitForActiveProcess(const std::string& exePath, const std::string& argsPattern) {
        if (argsPattern.empty()) {
            info("-- Waiting for process: {}", exePath);
        } else {
            info("-- Waiting for process: {}\n   with args: {}", exePath, argsPattern);
        }
        auto wExePath = StringToWString(exePath);
        auto wArgsPattern = StringToWString(argsPattern);
        while (!ProcessExists(wExePath, wArgsPattern)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}