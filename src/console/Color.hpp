#pragma once

#include <string>
#include <string_view>

namespace {
    bool noColor = false;
}

namespace devkit::Console::Color {
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

    inline void SetNoColorMode() {
        noColor = true;
    }

    inline std::string Colored(const std::string& text, const std::string_view& colorCode) {
        if (noColor) {
            return text;
        }
        return std::string(colorCode) + text + std::string(RESET);
    }

    inline std::string Green(const std::string& text) {
        return Colored(text, GREEN);
    }

    inline std::string Red(const std::string& text) {
        return Colored(text, RED);
    }

    inline std::string Cyan(const std::string& text) {
        return Colored(text, CYAN);
    }

    inline std::string Yellow(const std::string& text) {
        return Colored(text, YELLOW);
    }

    inline std::string Gray(const std::string& text) {
        return Colored(text, BRIGHT_BLACK);
    }
}