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
#include <task/Task.hpp>
#include <console/Utf8Guard.hpp>
#include <util/StringUtils.hpp>

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
            int timeout,
            std::vector<int> exitCodes,
            std::vector<std::string> commands,
            std::optional<std::filesystem::path> workingDirectory,
            std::optional<std::unordered_map<std::string, std::string>> env
        ) : Task(name, hidden, utf8, dependsOn, before, after, timeout, exitCodes),
            commands(std::move(commands)),
            workingDirectory(std::move(workingDirectory)),
            env(std::move(env)) {
        }

        void Run() const override {
            Utf8Guard utf8guard(utf8);
            std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();

            RunResult result = ShellRunner::Run(StringUtils::JoinShellCommands(commands), cwd, env.value_or(std::unordered_map<std::string, std::string>()), false, timeout);
            if (result.timedOut) {
                throw std::runtime_error("Task " + name + " timed out");
            }
            if (!IsValidExitCode(result)) {
                throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(result.exitCode.value_or(-1)));
            }
        }

    private:
        const std::vector<std::string> commands;
        const std::optional<std::filesystem::path> workingDirectory;
        const std::optional<std::unordered_map<std::string, std::string>> env;
    };
}