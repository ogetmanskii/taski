#pragma once

#include "../processes/processes.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <cwctype>
#include <cctype>
#include <algorithm>
#include <functional>
#include <sstream>
#include <optional>
#include <string_view>


namespace devkit {

    std::wstring StringToWString(const std::string& str);

    std::string WStringToString(const std::wstring& wstr);

    std::string TrimToSingleLine(const std::string& string);

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

    std::string ReadFileUtf8(const std::filesystem::path& filePath);

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
    
    // Запускает команду и ожидает ее завершения
    // Пример команды: "sample-env\test-executable --sleep 3 --print Hello World"
    //  в рабочей директории: "C:/Projects/sample-env"
    int RunShellCommand(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment,
        bool createNewProcessGroup = true
    );

    // Запускает команду и ожидает ее завершения. Отсоединяется после detachAfterSeconds
    std::optional<int> RunShellCommand(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment, 
        int detachAfterSeconds
    );

    // Запускает команду и ожидает ее завершения. Отсоединяется после того, как дочерний процесс напишет строку, содержащую detachAfterMessage
    std::optional<int> RunShellCommand(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment, 
        const std::string& detachAfterMessage
    );

    void WaitForNoActiveProcess(const std::string& exePath, const std::string& argsPattern = "*");

    void WaitForActiveProcess(const std::string& exePath, const std::string& argsPattern = "*");

    static inline std::string NormalizePath(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(), '\\', '/');
        //std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    static inline std::wstring NormalizePath(const std::wstring& path) {
        std::wstring result = path;
        std::replace(result.begin(), result.end(), L'\\', L'/');
        //std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }
}