#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <memory>
#include <exception>
#include <chrono>
#include <thread>

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

        // Возвращает true, если сервис healthy, иначе false
        bool WaitForHealthy() const {
            if (!definition.healthcheck.has_value()) {
                return true; // Если healthcheck не задан, считаем сервис здоровым
            }
            HealthcheckDefinition healthcheck = *definition.healthcheck;
            Utf8Guard utf8(healthcheck.utf8.value_or(definition.utf8));
            std::string workingDirectory = healthcheck.workingDirectory.value_or(definition.workingDirectory);
            std::unordered_map<std::string, std::string> env = healthcheck.environment.value_or(definition.environment);
            std::string command = JoinShellCommands(healthcheck.command);
            auto startTime = std::chrono::steady_clock::now();
            while (true) {
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
                if (elapsed >= healthcheck.timeout) {
                    return false;
                }
                RunResult result = ShellRunner::Run(
                    command,
                    workingDirectory,
                    env,
                    false,
                    healthcheck.timeout
                );
                if (ShellRunner::IsValidExitCode(result, healthcheck.exitCodes)) {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::seconds(healthcheck.interval));
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