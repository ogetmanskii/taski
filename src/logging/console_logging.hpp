#pragma once

#include <string_view>
#include <format>
#include <iostream>
#include <utility>
#include <string>
#include <sstream>

namespace devkit {
    void InitConsole();

    template<typename... Args>
    void info(std::string_view fmt, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            std::cout << fmt << std::endl;
        } else {
            std::cout << std::vformat(fmt, std::make_format_args(args...)) << std::endl;
        }
    }

    void ClearScreen();

    class DurationFormatter {
    public:
        static std::string Format(long long millis) {
            if (millis < 0) millis = 0;

            long long totalSeconds = millis / 1000;
            long long days = totalSeconds / 86400;
            long long hours = (totalSeconds % 86400) / 3600;
            long long minutes = (totalSeconds % 3600) / 60;
            long long seconds = totalSeconds % 60;
            long long remainingMillis = millis % 1000;

            std::ostringstream result;

            AppendIfNonZero(result, days, "d");
            AppendIfNonZero(result, hours, "h");
            AppendIfNonZero(result, minutes, "m");

            if (seconds > 0 || remainingMillis > 0 || result.tellp() == 0) {
                if (remainingMillis > 0) {
                    result << seconds << "." << remainingMillis << "s";
                } else {
                    AppendIfNonZero(result, seconds, "s");
                }
            }

            if (result.tellp() == 0) {
                return "0s";
            }

            std::string str = result.str();
            
            if (!str.empty() && str.back() == ' ') {
                str.pop_back();
            }
            return str;
        }

    private:
        static void AppendIfNonZero(std::ostringstream& oss, long long value, const char* suffix) {
            if (value > 0) {
                oss << value << suffix << " ";
            }
        }
    };
}

namespace color {
    // Базовые цвета текста
    constexpr std::string_view RED = "\033[31m";
    constexpr std::string_view GREEN = "\033[32m";
    constexpr std::string_view YELLOW = "\033[33m";
    constexpr std::string_view BLUE = "\033[34m";
    constexpr std::string_view MAGENTA = "\033[35m";
    constexpr std::string_view CYAN = "\033[36m";
    constexpr std::string_view WHITE = "\033[37m";

    // Яркие цвета
    constexpr std::string_view BRIGHT_RED = "\033[91m";
    constexpr std::string_view BRIGHT_GREEN = "\033[92m";
    constexpr std::string_view BRIGHT_YELLOW = "\033[93m";
    constexpr std::string_view BRIGHT_BLACK = "\033[90m";

    // Стили
    constexpr std::string_view BOLD = "\033[1m";
    constexpr std::string_view UNDERLINE = "\033[4m";
    constexpr std::string_view RESET = "\033[0m";

    // Фоновые цвета
    constexpr std::string_view BG_RED = "\033[41m";
    constexpr std::string_view BG_GREEN = "\033[42m";

    inline std::string colored(const std::string& text, const std::string_view& colorCode) {
        return std::string(colorCode) + text + std::string(RESET);
    }

    inline std::string green(const std::string& text) {
        return colored(text, GREEN);
    }

    inline std::string red(const std::string& text) {
        return colored(text, RED);
    }

    inline std::string cyan(const std::string& text) {
        return colored(text, CYAN);
    }

    inline std::string yellow(const std::string& text) {
        return colored(text, YELLOW);
    }

    inline std::string gray(const std::string& text) {
        return colored(text, BRIGHT_BLACK);
    }
}