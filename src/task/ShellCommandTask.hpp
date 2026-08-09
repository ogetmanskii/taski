#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <optional>
#include <exception>
#include <algorithm>
#include <memory>

#include "Task.hpp"
#include "../shell/ShellRunner.hpp"
#include "../util/StringUtils.hpp"

namespace devkit {
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
            env(std::move(env)) {
        }

        void Run() const override {
            Utf8Guard utf8guard(utf8);
            std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();

            int exitCode = ShellRunner::Run(StringUtils::JoinShellCommands(commands), cwd, env.value_or(std::unordered_map<std::string, std::string>()), false);
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