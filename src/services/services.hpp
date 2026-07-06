#pragma once
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <exception>
#include "../logging/console_logging.hpp"
#include "../processes/processes.hpp"
#include "../util/util.hpp"

namespace devkit {

    struct ServiceDefinition {
        std::string name;
        std::vector<std::string> dependsOn;
        std::string workingDirectory;
        bool utf8;
        std::unordered_map<std::string, std::string> environment;
        std::string startCommand;
        std::optional<std::string> stopCommand;

        std::optional<int> detachAfterSeconds;
        std::optional<std::string> detachAfterMessage;

        std::optional<std::string> monitorProcess;
        std::optional<std::string> monitorProcessArgsPattern;
    };

    enum ServiceStatus {
        UP,
        DOWN,
        UNKNOWN
    };

    class Service {
    public:
        const ServiceDefinition definition;

        Service() = delete;

        Service(ServiceDefinition definition)
            : definition(std::move(definition)) {
        }

        ServiceStatus Status(const std::vector<ProcessInfo>& activeProcesses) const {
            if (!definition.monitorProcess) {
                return ServiceStatus::UNKNOWN;
            }
            std::string processPath = *definition.monitorProcess;
            std::string processArgs = definition.monitorProcessArgsPattern.value_or("*");
            if (ProcessExists(activeProcesses, StringToWString(processPath), StringToWString(processArgs))) {
                return ServiceStatus::UP;
            } else {
                return ServiceStatus::DOWN;
            }
        }

        void Stop(const std::vector<ProcessInfo>& activeProcesses) const {
            Utf8Guard utf8(definition.utf8);
            if (definition.monitorProcess && !ProcessExists(
                activeProcesses,
                StringToWString(*definition.monitorProcess),
                StringToWString(definition.monitorProcessArgsPattern.value_or("*")))) {
                return;
            }
            if (definition.stopCommand.has_value()) {
                RunShellCommand(
                    definition.stopCommand.value(),
                    definition.workingDirectory,
                    definition.environment
                );
            } else if (definition.monitorProcess) {
                TerminateProcesses(
                    StringToWString(*definition.monitorProcess),
                    StringToWString(definition.monitorProcessArgsPattern.value_or("*"))
                );
            } else {
                info("-- Can not stop {} - no stopCommand and no monitorProcess specified", definition.name);
                return;
            }
            if (definition.monitorProcess) {
                WaitForNoActiveProcess(*definition.monitorProcess, definition.monitorProcessArgsPattern.value_or("*"));
            }
        }

        void Start(const std::vector<ProcessInfo>& activeProcesses) const {
            Utf8Guard utf8(definition.utf8);
            if (definition.monitorProcess.has_value() && ProcessExists(
                activeProcesses,
                StringToWString(definition.monitorProcess.value()),
                StringToWString(definition.monitorProcessArgsPattern.value_or("*")))) {
                return;
            }
            if (definition.startCommand.empty()) {
                throw std::runtime_error("Can not start " + definition.name + ": startCommand is empty");
            }
            if (definition.detachAfterMessage.has_value()) {
                std::optional<int> exitCode = RunShellCommand(
                    definition.startCommand,
                    definition.workingDirectory,
                    definition.environment,
                    definition.detachAfterMessage.value()
                );
                info("\n");
                if (exitCode.has_value() && (*exitCode) != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(*exitCode));
                }
            } else if (definition.detachAfterSeconds.has_value()) {
                std::optional<int> exitCode = RunShellCommand(
                    definition.startCommand,
                    definition.workingDirectory,
                    definition.environment,
                    definition.detachAfterSeconds.value()
                );
                info("\n");
                if (exitCode.has_value() && (*exitCode) != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(*exitCode));
                }
                if (definition.monitorProcess.has_value()) {
                    WaitForActiveProcess(*definition.monitorProcess, definition.monitorProcessArgsPattern.value_or("*"));
                    return;
                }
            } else {
                int exitCode = RunShellCommand(
                    definition.startCommand,
                    definition.workingDirectory,
                    definition.environment,
                    true
                );
                info("\n");
                if (exitCode != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(exitCode));
                }
            }
        }

    };
}