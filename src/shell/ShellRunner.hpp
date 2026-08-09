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

namespace devkit::ShellRunner {
    
    // Запускает команду и ожидает ее завершения
    // Пример команды: "sample-env\test-executable --sleep 3 --print Hello World"
    //  в рабочей директории: "C:/Projects/sample-env"
    int Run(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment,
        bool createNewProcessGroup = true
    );

    // Запускает команду и ожидает ее завершения. Отсоединяется после detachAfterSeconds
    std::optional<int> Run(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment, 
        int detachAfterSeconds
    );

    // Запускает команду и ожидает ее завершения. Отсоединяется после того, как дочерний процесс напишет строку, содержащую detachAfterMessage
    std::optional<int> Run(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment, 
        const std::string& detachAfterMessage
    );
}