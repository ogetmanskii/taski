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

    void ExecuteLuaFile(const std::string& file);

    enum FileTaskType {
        LUA,
        BAT,
        SH
    };

    class Task {
    public:
        Task(
            std::string name, 
            bool hidden, 
            std::vector<std::string> dependsOn,
            std::vector<std::string> before,
            std::vector<std::string> after,
            std::vector<int> exitCodes
        ) :
            name(std::move(name)),
            hidden(hidden),
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
        const std::vector<std::string> dependsOn;
        const std::vector<std::string> before;
        const std::vector<std::string> after;
        const std::vector<int> exitCodes;

        bool IsValidExitCode(int exitCode) const {
            return (exitCodes.empty() && exitCode == 0)
                || (std::find(exitCodes.begin(), exitCodes.end(), exitCode) != exitCodes.end());
        }
    };

    class FileTask : public Task {
    public:
        FileTask(
            std::string name,
            bool hidden,
            std::vector<std::string> dependsOn,
            std::vector<std::string> before,
            std::vector<std::string> after,
            std::vector<int> exitCodes,
            FileTaskType fileType,
            std::filesystem::path filePath
        ) : Task(name, hidden, dependsOn, before, after, exitCodes),
            filePath(std::move(filePath)),
            fileType(fileType)
        { }

        void Run() const override {
            if (fileType == FileTaskType::LUA) {
                try {
                    ExecuteLuaFile(filePath.string());
                } catch (const std::exception& e) {
                    throw std::runtime_error("Task " + name + " exited with error: " + e.what());
                }
            } else if (fileType == FileTaskType::BAT) {
                std::unordered_map<std::string, std::string> env;
                int exitCode = RunShellCommand("\"" + filePath.filename().string() + "\"", std::filesystem::current_path().string(), env, false);
                if (!IsValidExitCode(exitCode)) {
                    throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(exitCode));
                }
            } else if (fileType == FileTaskType::SH) {
                std::unordered_map<std::string, std::string> env;
                int exitCode = RunShellCommand("wsl \"./" + filePath.filename().string() + "\"", std::filesystem::current_path().string(), env, false);
                if (!IsValidExitCode(exitCode)) {
                    throw std::runtime_error("Task " + name + " exited with code: " + std::to_string(exitCode));
                }
            }
        }

    private:
        const FileTaskType fileType;
        const std::filesystem::path filePath;
    };

    class ShellCommandTask : public Task {
    public:
        ShellCommandTask(
            std::string name,
            bool hidden,
            std::vector<std::string> dependsOn,
            std::vector<std::string> before,
            std::vector<std::string> after,
            std::vector<int> exitCodes,
            std::vector<std::string> commands,
            std::optional<std::filesystem::path> workingDirectory,
            std::optional<std::unordered_map<std::string, std::string>> env
        ) : Task(name, hidden, dependsOn, before, after, exitCodes),
            commands(std::move(commands)),
            workingDirectory(std::move(workingDirectory)),
            env(std::move(env))
        { }

        void Run() const override {
            std::string cwd = workingDirectory.value_or(std::filesystem::current_path()).string();
            std::stringstream finalCommand;
            for (int i = 0; i < commands.size(); i++) {
                const std::string& command { commands[i] };
                if (i == 0) {
                    finalCommand << command;
                } else {
                    finalCommand << " && " << command;
                }
            }

            int exitCode = RunShellCommand(finalCommand.str(), cwd, env.value_or(std::unordered_map<std::string, std::string>()), false);
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