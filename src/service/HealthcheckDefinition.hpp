#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

namespace devkit {

    struct HealthcheckDefinition {
        // Команда для проверки
        std::vector<std::string> command;

        std::optional<std::string> workingDirectory;
        std::optional<bool> utf8;
        std::optional<std::unordered_map<std::string, std::string>> environment;

        // Успешные коды выхода команды проверки
        std::vector<int> exitCodes;

        // Интервал между вызовами команды
        int interval;
        // Таймаут в секундах, после которого, если сервис всё еще не healthy, то выбрасывать исключение
        int timeout;
    };

}