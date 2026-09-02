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

#include <processes/Processes.hpp>
#include <processes/ProcessFilter.hpp>
#include <processes/ProcessDescriptor.hpp>
#include <ShellRunner.hpp>
#include <service/ServiceDefinition.hpp>
#include <console/Console.hpp>
#include <util/StringUtils.hpp>

namespace devkit {

    using namespace Console;
    using namespace StringUtils;
    using namespace ShellRunner;
    using namespace Processes;

    enum ServiceStatus {
        UP,
        DOWN,
        UNKNOWN
    };

    class Service {
    public:
        ServiceDefinition definition;

        Service() = delete;

        Service(ServiceDefinition definition)
            : definition(std::move(definition)) {
        }

        ServiceStatus Status(std::vector<Processes::ProcessDescriptor>& activeProcesses) {
            if (!definition.processFilter) {
                return ServiceStatus::UNKNOWN;
            }
            if (Processes::ProcessExists(activeProcesses, *definition.processFilter)) {
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
            std::string workingDirectory = healthcheck.workingDirectory.value_or(definition.workingDirectory);
            std::unordered_map<std::string, std::string> env = healthcheck.environment.value_or(definition.environment);
            std::string command = JoinShellCommands(healthcheck.command);
            auto startTime = std::chrono::steady_clock::now();

            RunSpec spec = RunSpec {
                .command = command,
                .workingDirectory = workingDirectory,
                .environment = env,
                .utf8 = healthcheck.utf8.value_or(definition.utf8),
                .createNewProcessGroup = false,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeoutSeconds = healthcheck.timeout
            };
            while (true) {
                auto currentTime = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
                if (elapsed >= healthcheck.timeout) {
                    return false;
                }
                RunResult result = ShellRunner::Run(spec);
                if (ShellRunner::IsValidExitCode(result, healthcheck.exitCodes)) {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::seconds(healthcheck.interval));
            }
        }

        void Stop(std::vector<Processes::ProcessDescriptor>& activeProcesses) {
            if (definition.processFilter && !Processes::ProcessExists(activeProcesses, *definition.processFilter)) {
                return;
            }
            if (definition.stopCommand) {
                RunSpec spec = GetStopCommandSpec();
                RunResult result = ShellRunner::Run(spec);
                ValidateRunResult("Stop command", result, *definition.stopCommand);
            } else if (definition.processFilter) {
                Processes::TerminateProcesses(*definition.processFilter);
            } else {
                Info("-- Can not stop {} - no stop-command and no process-filter specified", definition.name);
                return;
            }
            if (definition.processFilter) {
                Processes::WaitForNoActiveProcess(*definition.processFilter);
            }
        }

        void Start(std::vector<Processes::ProcessDescriptor>& activeProcesses) {
            if (definition.processFilter && Processes::ProcessExists(activeProcesses, *definition.processFilter)) {
                // Already UP
                return;
            }
            
            RunSpec spec = GetStartCommandSpec();
            RunResult result = ShellRunner::Run(spec);
            ValidateRunResult("Start command", result, definition.startCommand);

            if (definition.processFilter) {
                Processes::WaitForActiveProcess(*definition.processFilter);
                return;
            }
        }

    private:
        RunSpec GetStartCommandSpec() {
            const ServiceCommandDefinition& cmdDef = definition.startCommand;
            return RunSpec {
                .command = JoinShellCommands(cmdDef.command),
                .workingDirectory = cmdDef.workingDirectory.value_or(definition.workingDirectory),
                .environment = cmdDef.environment.value_or(definition.environment),
                .utf8 = cmdDef.utf8.value_or(definition.utf8),
                .createNewProcessGroup = cmdDef.createNewProcessGroup.value_or(true),
                .detachAfterSeconds = cmdDef.detachAfterSeconds,
                .detachAfterMessage = cmdDef.detachAfterMessage,
                .timeoutSeconds = cmdDef.timeout.value_or(-1)
            };
        }

        RunSpec GetStopCommandSpec() {
            const ServiceCommandDefinition& cmdDef = *definition.stopCommand;
            return RunSpec {
                .command = JoinShellCommands(cmdDef.command),
                .workingDirectory = cmdDef.workingDirectory.value_or(definition.workingDirectory),
                .environment = cmdDef.environment.value_or(definition.environment),
                .utf8 = cmdDef.utf8.value_or(definition.utf8),
                .createNewProcessGroup = cmdDef.createNewProcessGroup.value_or(false),
                .detachAfterSeconds = cmdDef.detachAfterSeconds,
                .detachAfterMessage = cmdDef.detachAfterMessage,
                .timeoutSeconds = cmdDef.timeout.value_or(-1)
            };
        }

        void ValidateRunResult(const std::string& context, const RunResult& result, const ServiceCommandDefinition& cmdDef) {
            if (result.timedOut) {
                throw std::runtime_error("Start command timed out");
            }
            if (result.exitCode && !IsValidExitCode(result, cmdDef.exitCodes)) {
                throw std::runtime_error("Start command exited with code: " + std::to_string(*result.exitCode));
            }
            if (result.errorCode) {
                throw std::runtime_error("Start command failed: error " + std::to_string(*result.errorCode));
            }
        }
    };
}