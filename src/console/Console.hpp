#pragma once

#include <string_view>
#include <format>
#include <iostream>
#include <utility>
#include <string>
#include <sstream>

namespace devkit::Console {

    void Init();

    template<typename... Args>
    void Info(std::string_view fmt, Args&&... args) {
        if constexpr (sizeof...(args) == 0) {
            std::cout << fmt << std::endl;
        } else {
            std::cout << std::vformat(fmt, std::make_format_args(args...)) << std::endl;
        }
    }

    void ClearScreen();

    void TransformUtf8Strings(bool transform);

    std::pair<int /* width */, int /* height */> GetConsoleBounds();

    /**
     * Подсчет печатной длины строки с учетом ANSI escape-последовательностей
     */
    inline int GetTerminalLength(const std::string_view str) {
        int length = 0;
        size_t i = 0;

        while (i < str.size()) {
            if (i + 1 < str.size() && str[i] == '\033' && str[i + 1] == '[') {
                // Пропускаем escape-последовательность
                i += 2;
                while (i < str.size() && !std::isalpha(static_cast<unsigned char>(str[i]))) {
                    ++i;
                }
                if (i < str.size()) {
                    ++i; // пропускаем завершающую букву
                }
            } else {
                // Определяем длину UTF-8 символа
                size_t char_len = 1;
                unsigned char c = static_cast<unsigned char>(str[i]);

                if (c >= 0xC0 && c < 0xE0) char_len = 2;
                else if (c >= 0xE0 && c < 0xF0) char_len = 3;
                else if (c >= 0xF0 && c < 0xF8) char_len = 4;

                if (i + char_len <= str.size()) {
                    ++length;
                    i += char_len;
                } else {
                    ++i; // некорректная последовательность
                }
            }
        }
        return length;
    }
}