#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <processes/ProcessFilter.hpp>
#include <util/YamlUtils.hpp>
#include <util/StringUtils.hpp>
#include <util/FileUtils.hpp>
#include <util/TemplateUtils.hpp>
#include <task/Task.hpp>
#include <service/Service.hpp>
#include <service/ServiceDefinition.hpp>
#include <parser/EnvironmentParser.hpp>

namespace {

    using namespace devkit;
    using namespace devkit::YamlUtils;
    using namespace devkit::TemplateUtils;
    using namespace devkit::Processes;

    std::optional<HealthcheckDefinition> ParseHealthcheck(const YAML::Node& healthcheckNode) {
        if (!healthcheckNode) {
            return std::nullopt;
        }
        if (!healthcheckNode.IsMap()) {
            return std::nullopt;
        }
        return HealthcheckDefinition {
            .command = GetShellCommandSequence(healthcheckNode["cmd"]),
            .workingDirectory = GetOptionalScalar<std::string>(healthcheckNode["work-dir"]),
            .utf8 = GetOptionalScalar<bool>(healthcheckNode["utf8"]),
            .environment = GetOptionalMap<std::string, std::string>(healthcheckNode["env"]),
            .exitCodes = GetList<int>(healthcheckNode["exit-codes"]),
            .interval = GetOptionalScalar<int>(healthcheckNode["interval"]).value_or(5),
            .timeout = GetOptionalScalar<int>(healthcheckNode["timeout"]).value_or(30)
        };
    }

    ServiceCommandDefinition ParseStartCommand(const YAML::Node& node, const ServiceDefinition& serviceDef) {
        if (!node) {
            throw std::runtime_error("Could not parse service " + serviceDef.name + ": no start command specified");
        }
        if (node.IsScalar()) {
            return ServiceCommandDefinition {
                .command = { node.as<std::string>() },
                .workingDirectory = std::nullopt,
                .environment = std::nullopt,
                .utf8 = std::nullopt,
                .createNewProcessGroup = true,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeout = -1,
                .exitCodes = { 0 }
            };
        } else if (node.IsSequence()) {
            return ServiceCommandDefinition {
                .command = GetShellCommandSequence(node),
                .workingDirectory = std::nullopt,
                .environment = std::nullopt,
                .utf8 = std::nullopt,
                .createNewProcessGroup = true,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeout = -1,
                .exitCodes = { 0 }
            };
        } else if (node.IsMap()) {
            return ServiceCommandDefinition {
                .command = GetShellCommandSequence(node["cmd"]),
                .workingDirectory = GetOptionalScalar<std::string>(node["work-dir"]),
                .environment = GetOptionalMap<std::string, std::string>(node["env"]),
                .utf8 = GetOptionalScalar<bool>(node["utf8"]),
                .createNewProcessGroup = GetOptionalScalar<bool>(node["create-new-process-group"]),
                .detachAfterSeconds = GetOptionalScalar<int>(node["detach-after-seconds"]),
                .detachAfterMessage = GetOptionalScalar<std::string>(node["detach-after-message"]),
                .timeout = GetInt(node["timeout"], -1),
                .exitCodes = GetList<int>(node["exit-codes"])
            };
        } else {
            throw std::runtime_error("Could not parse start-command: unknown format");
        }
    }

    ServiceCommandDefinition ParseStopCommand(const YAML::Node& node, const ServiceDefinition& serviceDef) {
        if (!node) {
            throw std::runtime_error("Could not parse service " + serviceDef.name + ": no stop command specified");
        }
        if (node.IsScalar()) {
            return ServiceCommandDefinition {
                .command = { node.as<std::string>() },
                .workingDirectory = std::nullopt,
                .environment = std::nullopt,
                .utf8 = std::nullopt,
                .createNewProcessGroup = false,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeout = -1,
                .exitCodes = { 0 }
            };
        } else if (node.IsSequence()) {
            return ServiceCommandDefinition {
                .command = GetShellCommandSequence(node),
                .workingDirectory = std::nullopt,
                .environment = std::nullopt,
                .utf8 = std::nullopt,
                .createNewProcessGroup = false,
                .detachAfterSeconds = std::nullopt,
                .detachAfterMessage = std::nullopt,
                .timeout = -1,
                .exitCodes = { 0 }
            };
        } else if (node.IsMap()) {
            return ServiceCommandDefinition {
                .command = GetShellCommandSequence(node["cmd"]),
                .workingDirectory = GetOptionalScalar<std::string>(node["work-dir"]),
                .environment = GetOptionalMap<std::string, std::string>(node["env"]),
                .utf8 = GetOptionalScalar<bool>(node["utf8"]),
                .createNewProcessGroup = GetOptionalScalar<bool>(node["create-new-process-group"]),
                .detachAfterSeconds = GetOptionalScalar<int>(node["detach-after-seconds"]),
                .detachAfterMessage = GetOptionalScalar<std::string>(node["detach-after-message"]),
                .timeout = GetInt(node["timeout"], -1),
                .exitCodes = GetList<int>(node["exit-codes"])
            };
        } else {
            throw std::runtime_error("Could not parse stop-command: unknown format");
        }
    }

    std::optional<ProcessFilter> ParseProcessFilter(const YAML::Node& node) {
        if (!node) {
            return std::nullopt;
        }
        ProcessFilter filter;
        if (node.IsScalar()) {
            filter.WithExecutablePattern(node.as<std::string>());
        } else if (node.IsMap()) {
            auto executableNode = node["exe"];
            if (executableNode && executableNode.IsScalar()) {
                filter.WithExecutablePattern(executableNode.as<std::string>());
            }
            auto argsNode = node["args"];
            if (argsNode && argsNode.IsScalar()) {
                filter.WithCommandLineArgsPattern(argsNode.as<std::string>());
            }
            auto wdNode = node["work-dir"];
            if (wdNode && wdNode.IsScalar()) {
                filter.WithWorkingDirectoryPattern(wdNode.as<std::string>());
            }
        }
        return filter;
    }

    ServiceDefinition ParseService(const std::string& tagName, const YAML::Node& serviceNode) {
        ServiceDefinition serviceDef;
        serviceDef.name = GetOptionalScalar<std::string>(serviceNode["name"]).value_or(tagName);
        serviceDef.workingDirectory = OrWorkingDirectory(serviceNode["work-dir"]);
        serviceDef.environment = GetMap(serviceNode["env"]);
        serviceDef.utf8 = GetBool(serviceNode["utf8"], false);
        serviceDef.dependsOn = GetList<std::string>(serviceNode["depends-on"]);
        serviceDef.startCommand = ParseStartCommand(serviceNode["start"], serviceDef);
        if (serviceNode["stop"]) {
            serviceDef.stopCommand = ParseStopCommand(serviceNode["stop"], serviceDef);
        } else {
            serviceDef.stopCommand = std::nullopt;
        }
        serviceDef.processFilter = ParseProcessFilter(serviceNode["process-filter"]);
        serviceDef.healthcheck = ParseHealthcheck(serviceNode["healthcheck"]);
        return serviceDef;
    }

    void ParseServices(
        const YAML::Node& servicesNode,
        std::vector<std::shared_ptr<devkit::Service>>& outServices) {

        for (auto it = servicesNode.begin(); it != servicesNode.end(); it++) {
            const std::string& serviceTag = it->first.as<std::string>();
            const YAML::Node& serviceNode = it->second;
            if (!serviceNode.IsMap()) {
                continue;
            }
            try {
                auto definition = ParseService(serviceTag, serviceNode);
                outServices.push_back(std::make_shared<Service>(std::move(definition)));
            } catch (const std::exception& e) {
                Info("Could not parse service: {}", e.what());
            }
        }
    }

    std::shared_ptr<Task> ParseTask(const std::string& tag, const YAML::Node& node) {
        std::string name = GetOptionalScalar<std::string>(node["name"]).value_or(tag);
        std::filesystem::path workingDirectory = OrWorkingDirectory(node["work-dir"]);
        std::vector<std::string> dependsOn = GetList<std::string>(node["depends-on"]);
        std::vector<std::string> before = GetList<std::string>(node["before"]);
        std::vector<std::string> after = GetList<std::string>(node["after"]);
        bool hidden = GetBool(node["hidden"], false);
        bool utf8 = GetBool(node["utf8"], false);
        int timeout = GetOptionalScalar<int>(node["timeout"]).value_or(-1);
        std::vector<int> exitCodes = GetList<int>(node["exit-codes"]);

        auto& commandNode = node["cmd"];
        if (!commandNode) {
            throw std::runtime_error("Could not parse task " + name + ": no command specified");
        }
        
        std::vector<std::string> commands = GetShellCommandSequence(commandNode);
        std::unordered_map<std::string, std::string> taskEnv = GetMap(node["env"]);
        
        return std::make_shared<Task>(
            std::move(name),
            hidden,
            utf8,
            std::move(dependsOn),
            std::move(before),
            std::move(after),
            timeout,
            std::move(exitCodes),
            std::move(commands),
            std::move(workingDirectory),
            std::move(taskEnv)
        );
    }

    void ParseTasks(const YAML::Node& tasksNode, std::vector<std::shared_ptr<Task>>& outTasks) {
        for (auto it = tasksNode.begin(); it != tasksNode.end(); it++) {
            const std::string& taskTag = it->first.as<std::string>();
            const YAML::Node& taskNode = it->second;
            if (!taskNode.IsMap()) {
                continue;
            }
            try {
                outTasks.push_back(ParseTask(taskTag, taskNode));
            } catch (const std::exception& e) {
                Info("Could not parse task: {}", e.what());
            }
        }
    }
}

namespace devkit::Parser {

    void ParseEnvironmentYmlFile(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks) {

        std::string yaml = RenderTemplate(FileUtils::ReadFileUtf8(filePath), env);

        YAML::Node rootNode = YAML::Load(yaml);
        if (!rootNode.IsMap()) {
            return;
        }

        YAML::Node servicesNode = rootNode["services"];
        if (servicesNode.IsMap()) {
            ParseServices(servicesNode, outServices);
        }

        YAML::Node tasksNode = rootNode["tasks"];
        if (tasksNode.IsMap()) {
            ParseTasks(tasksNode, outTasks);
        }
    }
}