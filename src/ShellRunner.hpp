#pragma once

#include <string>
#include <unordered_map>
#include <optional>

namespace devkit::ShellRunner {
    
    struct RunSpec {
        
        // Shell команда.
        const std::string command;
        
        // Рабочая директория.
        const std::string workingDirectory;
        
        // Дополнительные переменные окружения.
        const std::unordered_map<std::string, std::string> environment;

        // Вывод в UTF-8.
        // false - выводить STDOUT процесса в кодировке консоли по умолчанию.
        // true - преобразовать в UTF-8.
        const bool utf8;

        // Создавать ли новую группу процессов.
        // false - будет дочерний процесс от текущего процесса.
        // true - будет самостоятельный процесс.
        const bool createNewProcessGroup;

        // Отсоединиться от процесса после N секунд.
        // 0 - отсоединиться сразу после запуска команды.
        const std::optional<float> detachAfterSeconds;

        // Отсоединиться от процесса после получения указанного сообщения в STDOUT.
        const std::optional<std::string> detachAfterMessage;

        // Таймаут ожидания процесса.
        // Если указано -1, тогда таймаут отсутствует (ждем завершения команды бесконечно).
        // Игнорируется, если указано detachAfterSeconds.
        const float timeoutSeconds;
    };

    struct RunResult {

        // Код возврата команды (если была завершена).
        std::optional<int> exitCode;
        
        // Был ли таймаут ожидания команды.
        bool timedOut;

        // Код внутренней ошибки, если не удалось создать процесс или получить его код возврата.
        std::optional<unsigned int> errorCode;
    };

    // Запуск команды по спецификации
    RunResult Run(const RunSpec spec);

    inline bool IsValidExitCode(RunResult runResult, std::vector<int> expectedExitCodes) {
        if (!runResult.exitCode) {
            return false;
        }
        int exitCode = runResult.exitCode.value();
        return (expectedExitCodes.empty() && exitCode == 0)
            || (std::find(expectedExitCodes.begin(), expectedExitCodes.end(), exitCode) != expectedExitCodes.end());
    }
}