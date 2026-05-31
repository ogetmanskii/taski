#pragma once

#include <string_view>
#include <format>
#include <iostream>
#include <utility>

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
}