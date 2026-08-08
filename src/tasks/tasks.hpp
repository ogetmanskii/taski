#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <optional>
#include <exception>
#include <algorithm>
#include <memory>
#include "../util/util.hpp"

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
            std::vector<int> exitCodes
        ) :
            name(std::move(name)),
            hidden(hidden),
            utf8(utf8),
            dependsOn(std::move(dependsOn)),
            before(std::move(before)),
            after(std::move(after)),
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
        const std::vector<int> exitCodes;

        bool IsValidExitCode(int exitCode) const {
            return (exitCodes.empty() && exitCode == 0)
                || (std::find(exitCodes.begin(), exitCodes.end(), exitCode) != exitCodes.end());
        }
    };

    class ShellCommandTask : public Task {
    public:
        ShellCommandTask(
            std::string name,
            bool hidden,
            bool utf8,
            std::vector<std::string> dependsOn,
            std::vector<std::string> before,
            std::vector<std::string> after,
            std::vector<int> exitCodes,
            std::vector<std::string> commands,
            std::optional<std::filesystem::path> workingDirectory,
            std::optional<std::unordered_map<std::string, std::string>> env
        ) : Task(name, hidden, utf8, dependsOn, before, after, exitCodes),
            commands(std::move(commands)),
            workingDirectory(std::move(workingDirectory)),
            env(std::move(env))
        { }

        void Run() const override {
            Utf8Guard utf8guard(utf8);
            std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();

            int exitCode = RunShellCommand(JoinCommands(commands), cwd, env.value_or(std::unordered_map<std::string, std::string>()), false);
            if (!IsValidExitCode(exitCode)) {
                throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(exitCode));
            }
        }

    private:
        const std::vector<std::string> commands;
        const std::optional<std::filesystem::path> workingDirectory;
        const std::optional<std::unordered_map<std::string, std::string>> env;
    };
}