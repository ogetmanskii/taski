#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

namespace devkit {

    struct HealthcheckDefinition {
        
        // Команда для проверки
        // cmd
        std::vector<std::string> command;

        // work-dir
        std::optional<std::string> workingDirectory;

        // utf8
        std::optional<bool> utf8;

        // env
        std::optional<std::unordered_map<std::string, std::string>> environment;

        // Успешные коды выхода команды проверки
        // exit-codes
        std::vector<int> exitCodes;

        // Интервал в секундах между вызовами команды
        // interval
        float interval;

        // Таймаут в секундах, после которого, если сервис всё еще не healthy, то выбрасывать исключение
        // timeout
        float timeout;
    };

}