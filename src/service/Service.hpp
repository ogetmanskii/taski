#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <exception>

#include <Processes.hpp>
#include <ShellRunner.hpp>
#include <service/ServiceDefinition.hpp>
#include <console/Utf8Guard.hpp>
#include <console/Console.hpp>
#include <util/StringUtils.hpp>

namespace devkit {

    using namespace Console;
    using namespace StringUtils;
    using namespace ShellRunner;

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

        ServiceStatus Status(const std::vector<Processes::ProcessInfo>& activeProcesses) const {
            if (!definition.monitorProcess) {
                return ServiceStatus::UNKNOWN;
            }
            std::string processPath = *definition.monitorProcess;
            std::string processArgs = definition.monitorProcessArgsPattern.value_or("*");
            if (Processes::ProcessExists(activeProcesses, StringToWString(processPath), StringToWString(processArgs))) {
                return ServiceStatus::UP;
            } else {
                return ServiceStatus::DOWN;
            }
        }

        void Stop(const std::vector<Processes::ProcessInfo>& activeProcesses) const {
            Utf8Guard utf8(definition.utf8);
            if (definition.monitorProcess && !Processes::ProcessExists(
                activeProcesses,
                StringToWString(*definition.monitorProcess),
                StringToWString(definition.monitorProcessArgsPattern.value_or("*")))) {
                return;
            }
            if (!definition.stopCommand.empty()) {
                ShellRunner::Run(
                    JoinShellCommands(definition.stopCommand),
                    definition.workingDirectory,
                    definition.environment
                );
            } else if (definition.monitorProcess) {
                Processes::TerminateProcesses(
                    StringToWString(*definition.monitorProcess),
                    StringToWString(definition.monitorProcessArgsPattern.value_or("*"))
                );
            } else {
                Info("-- Can not stop {} - no stopCommand and no monitorProcess specified", definition.name);
                return;
            }
            if (definition.monitorProcess) {
                Processes::WaitForNoActiveProcess(*definition.monitorProcess, definition.monitorProcessArgsPattern.value_or("*"));
            }
        }

        void Start(const std::vector<Processes::ProcessInfo>& activeProcesses) const {
            Utf8Guard utf8(definition.utf8);
            if (definition.monitorProcess.has_value() && Processes::ProcessExists(
                activeProcesses,
                StringToWString(definition.monitorProcess.value()),
                StringToWString(definition.monitorProcessArgsPattern.value_or("*")))) {
                return;
            }
            if (definition.startCommand.empty()) {
                throw std::runtime_error("Can not start " + definition.name + ": startCommand is empty");
            }
            if (definition.detachAfterMessage.has_value()) {
                std::optional<int> exitCode = ShellRunner::Run(
                    JoinShellCommands(definition.startCommand),
                    definition.workingDirectory,
                    definition.environment,
                    definition.detachAfterMessage.value()
                );
                Info("\n");
                if (exitCode.has_value() && (*exitCode) != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(*exitCode));
                }
            } else if (definition.detachAfterSeconds.has_value()) {
                std::optional<int> exitCode = ShellRunner::Run(
                    JoinShellCommands(definition.startCommand),
                    definition.workingDirectory,
                    definition.environment,
                    definition.detachAfterSeconds.value()
                );
                Info("\n");
                if (exitCode.has_value() && (*exitCode) != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(*exitCode));
                }
                if (definition.monitorProcess.has_value()) {
                    Processes::WaitForActiveProcess(*definition.monitorProcess, definition.monitorProcessArgsPattern.value_or("*"));
                    return;
                }
            } else {
                RunResult result = ShellRunner::Run(
                    JoinShellCommands(definition.startCommand),
                    definition.workingDirectory,
                    definition.environment,
                    true
                );
                Info("\n");
                if (!result.exitCode) {
                    throw std::runtime_error("Start command failed with no exit code");
                }
                int exitCode = *result.exitCode;
                if (exitCode != 0) {
                    throw std::runtime_error("Start command exited with code: " + std::to_string(exitCode));
                }
            }
        }
    };
}