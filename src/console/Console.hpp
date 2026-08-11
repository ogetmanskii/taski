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
            // Проверяем начало escape-последовательности
            if (i + 1 < str.size() && str[i] == '\033' && str[i + 1] == '[') {
                // Пропускаем ESC [
                i += 2;

                // Пропускаем все символы до буквы-терминатора (обычно m, но могут быть и другие)
                while (i < str.size() && !std::isalpha(static_cast<unsigned char>(str[i]))) {
                    // Пропускаем цифры, точки с запятой и другие не-буквенные символы
                    if (str[i] >= '0' && str[i] <= '9' ||
                        str[i] == ';' || str[i] == ':' ||
                        str[i] == '<' || str[i] == '=' ||
                        str[i] == '>' || str[i] == '?' ||
                        str[i] == '[' || str[i] == ']') {
                        ++i;
                    } else {
                        // Если встретили что-то непонятное - считаем как обычный символ
                        ++length;
                        break;
                    }
                }

                // Пропускаем завершающую букву
                if (i < str.size() && std::isalpha(static_cast<unsigned char>(str[i]))) {
                    ++i;
                }
            } else {
                // Обычный печатный символ
                ++length;
                ++i;
            }
        }

        return length;
    }
}