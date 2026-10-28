#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cctype>
#include <algorithm>

namespace devkit::FileUtils {

inline std::string ReadFileUtf8(const std::filesystem::path& filePath) {
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
  if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
      static_cast<unsigned char>(content[1]) == 0xBB && static_cast<unsigned char>(content[2]) == 0xBF) {
    content.erase(0, 3);
  }

  return content;
}

inline std::string SanitizeFilename(std::string_view input) {
  std::string out;
  out.reserve(input.size());

  for (unsigned char c : input) {
    if (c < 0x20 || c == 0x7F) continue;
#ifdef _WIN32
    if (std::string_view("\\/:*?\"<>|").find(c) != std::string_view::npos) continue;
#else
    if (c == '/') continue;
#endif
    out.push_back(static_cast<char>(c));
  }

#ifdef _WIN32
  while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
#endif

#ifdef _WIN32
  static const char* reserved[] = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
                                   "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
                                   "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
  std::string upper = out;
  std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
  auto dot = upper.find('.');
  std::string stem = (dot == std::string::npos) ? upper : upper.substr(0, dot);
  for (auto r : reserved) {
    if (stem == r) {
      out = "_" + out;
      break;
    }
  }
#endif

  if (out.empty()) out = "unnamed";
  return out;
}

}  // namespace devkit::FileUtils