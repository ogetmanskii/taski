#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

namespace devkit {
    struct ServiceDefinition {
        std::string name;
        std::vector<std::string> dependsOn;
        std::string workingDirectory;
        bool utf8;
        std::unordered_map<std::string, std::string> environment;
        std::vector<std::string> startCommand;
        std::vector<std::string> stopCommand;

        std::optional<int> detachAfterSeconds;
        std::optional<std::string> detachAfterMessage;

        std::optional<std::string> monitorProcess;
        std::optional<std::string> monitorProcessArgsPattern;
    };
}