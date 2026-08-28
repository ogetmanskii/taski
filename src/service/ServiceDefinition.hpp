#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

#include <processes/ProcessFilter.hpp>
#include <service/HealthcheckDefinition.hpp>

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

        std::optional<Processes::ProcessFilter> monitorProcessFilter;

        std::optional<HealthcheckDefinition> healthcheck;
    };
}