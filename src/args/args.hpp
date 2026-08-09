#pragma once

#include <optional>
#include <string>
#include <vector>

namespace devkit {
    class Args {
    public:
        std::string currentPath;
        std::string environmentFile;
        std::string dotEnvFile;

        bool listCommand = false;

        bool upCommand = false;
        std::vector<std::string> upServicesList;

        bool downCommand = false;
        std::vector<std::string> downServicesList;
        
        std::vector<std::string> runList;

        bool printStatus = false;
        bool menu = false;

        bool versionCommand = false;

        std::optional<int> exitCode;

        const bool HasSpecificCommands() const {
            return listCommand 
                || upCommand
                || downCommand
                || printStatus
                || menu
                || versionCommand
                || !runList.empty();
        };

        static const Args FromArgv(int argc, char* argv[]);
    };
}