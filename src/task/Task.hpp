#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <optional>
#include <exception>
#include <algorithm>
#include <memory>

namespace devkit {

    class Task {
    public:
        Task(
            std::string name, 
            bool hidden, 
            bool utf8,
            std::vector<std::string> dependsOn,
            std::vector<std::string> before,
            std::vector<std::string> after,
            int timeout,
            std::vector<int> exitCodes
        ) :
            name(std::move(name)),
            hidden(hidden),
            utf8(utf8),
            dependsOn(std::move(dependsOn)),
            before(std::move(before)),
            after(std::move(after)),
            timeout(timeout),
            exitCodes(std::move(exitCodes))
        { }

        virtual ~Task() = default;
        virtual void Run() const = 0;

        const std::string& GetName() const {
            return name;
        }

        bool IsHidden() const {
            return hidden;
        }

        const std::vector<std::string>& GetDependsOn() const {
            return dependsOn;
        }

        const std::vector<std::string>& GetBefore() const {
            return before;
        }

        const std::vector<std::string>& GetAfter() const {
            return after;
        }

    protected:
        const std::string name;
        const bool hidden;
        const bool utf8;
        const std::vector<std::string> dependsOn;
        const std::vector<std::string> before;
        const std::vector<std::string> after;
        const int timeout;
        const std::vector<int> exitCodes;

        bool IsValidExitCode(RunResult runResult) const {
            if (!runResult.exitCode) {
                return false;
            }
            int exitCode = runResult.exitCode.value();
            return (exitCodes.empty() && exitCode == 0)
                || (std::find(exitCodes.begin(), exitCodes.end(), exitCode) != exitCodes.end());
        }
    };
}