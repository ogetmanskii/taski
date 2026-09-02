#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

#include <processes/ProcessFilter.hpp>
#include <service/HealthcheckDefinition.hpp>

namespace devkit {

    struct ServiceCommandDefinition {
        
        // cmd
        std::vector<std::string> command;
        
        // work-dir
        std::optional<std::string> workingDirectory;
        
        // env
        std::optional<std::unordered_map<std::string, std::string>> environment;
        
        // utf8
        std::optional<bool> utf8;
        
        // create-new-process-group
        std::optional<bool> createNewProcessGroup;
        
        // detach-after-seconds
        std::optional<float> detachAfterSeconds;
        
        // detach-after-message
        std::optional<std::string> detachAfterMessage;
        
        // timeout
        std::optional<float> timeout;

        // exit-codes
        std::vector<int> exitCodes;
    };

    struct ServiceDefinition {
        
        // name
        std::string name;
        
        // work-dir
        std::string workingDirectory;
        
        // env
        std::unordered_map<std::string, std::string> environment;

        // utf8
        bool utf8;

        // depends-on
        std::vector<std::string> dependsOn;

        // start
        ServiceCommandDefinition startCommand;

        // stop
        std::optional<ServiceCommandDefinition> stopCommand;

        // process-filter
        std::optional<Processes::ProcessFilter> processFilter;

        // healthcheck
        std::optional<HealthcheckDefinition> healthcheck;
    };
}