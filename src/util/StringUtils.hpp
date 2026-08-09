#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace devkit::StringUtils {

    inline std::string JoinShellCommands(const std::vector<std::string>& commands) {
        std::stringstream finalCommand;
        for (int i = 0; i < commands.size(); i++) {
            const std::string& command { commands[i] };
            if (i == 0) {
                finalCommand << command;
            } else {
                finalCommand << " && " << command;
            }
        }
        return finalCommand.str();
    }

    inline std::string TrimToSingleLine(const std::string& string) {
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

    std::wstring StringToWString(const std::string& str);

    std::string WStringToString(const std::wstring& wstr);

    template<typename T>
    std::string PadLeft(const T& value, size_t minWidth, char fill = ' ') {
        std::ostringstream oss;
        oss << std::left << std::setw(minWidth) << std::setfill(fill) << value;
        return oss.str();
    }

    template<typename T>
    std::string PadRight(const T& value, size_t minWidth, char fill = ' ') {
        std::ostringstream oss;
        oss << std::right << std::setw(minWidth) << std::setfill(fill) << value;
        return oss.str();
    }

}