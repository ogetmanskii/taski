#pragma once

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
#include "../processes/processes.hpp"

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

    void DownloadFile(
        const std::string& url, 
        const std::string& localPath, 
        bool ignoreSslErrors = false,
        bool useSystemCaBundle = true
    );

    void ExtractZipFile(
        const std::string& zipFilePath, 
        const std::string& destDir
    );
}