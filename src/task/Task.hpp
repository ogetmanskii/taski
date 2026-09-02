#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <optional>
#include <exception>
#include <algorithm>
#include <memory>

#include <ShellRunner.hpp>

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
            std::vector<int> exitCodes,
            std::vector<std::string> commands,
            std::optional<std::filesystem::path> workingDirectory,
            std::optional<std::unordered_map<std::string, std::string>> env
        ) :
            name(std::move(name)),
            hidden(hidden),
            utf8(utf8),
            dependsOn(std::move(dependsOn)),
            before(std::move(before)),
            after(std::move(after)),
            timeout(timeout),
            exitCodes(std::move(exitCodes)),
            commands(std::move(commands)),
            workingDirectory(std::move(workingDirectory)),
            env(std::move(env))
        { }

        void Run() const {
            std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();

            ShellRunner::RunSpec spec = {
                .command = StringUtils::JoinShellCommands(commands),
                .workingDirectory = cwd,
                .environment = env.value_or(std::unordered_map<std::string, std::string>()),
                .utf8 = utf8,
                .createNewProcessGroup = false,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeoutSeconds = timeout
            };

            ShellRunner::RunResult result = ShellRunner::Run(spec);
            if (result.timedOut) {
                throw std::runtime_error("Task " + name + " timed out");
            }
            if (!ShellRunner::IsValidExitCode(result, exitCodes)) {
                throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(*result.exitCode));
            }
            if (result.errorCode) {
                throw std::runtime_error("Task " + name + " failed: error " + std::to_string(*result.errorCode));
            }
        }

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
        // name
        const std::string name;

        // hidden
        const bool hidden;

        // utf8
        const bool utf8;

        // depends-on
        const std::vector<std::string> dependsOn;

        // before
        const std::vector<std::string> before;

        // after
        const std::vector<std::string> after;

        // cmd
        const std::vector<std::string> commands;

        // work-dir
        const std::optional<std::filesystem::path> workingDirectory;

        // env
        const std::optional<std::unordered_map<std::string, std::string>> env;

        // timeout
        const int timeout;

        // exit-codes
        const std::vector<int> exitCodes;
    };
}