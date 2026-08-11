#pragma once

#include <string>
#include <unordered_map>
#include <optional>

namespace devkit::ShellRunner {
    
    struct RunResult {
        std::optional<int> exitCode;
        bool timedOut;
        std::optional<unsigned int> errorCode;
    };

    // Запускает команду и ожидает ее завершения
    // Пример команды: "sample-env\test-executable --sleep 3 --print Hello World"
    //  в рабочей директории: "C:/Projects/sample-env"
    RunResult Run(
        const std::string& command, 
        const std::string& workingDirectory, 
        const std::unordered_map<std::string, std::string>& environment,
        bool createNewProcessGroup = true,
        int timeoutSeconds = -1
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