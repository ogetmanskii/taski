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
#include <service/Service.hpp>
#include <service/ServiceDefinition.hpp>
#include <task/ShellCommandTask.hpp>
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
            .command = GetShellCommandSequence(healthcheckNode["command"]),
            .workingDirectory = GetOptionalScalar<std::string>(healthcheckNode["workingDirectory"]),
            .utf8 = GetOptionalScalar<bool>(healthcheckNode["utf8"]),
            .environment = GetOptionalMap<std::string, std::string>(healthcheckNode["environment"]),
            .exitCodes = GetList<int>(healthcheckNode["exitCodes"]),
            .interval = GetOptionalScalar<int>(healthcheckNode["interval"]).value_or(5),
            .timeout = GetOptionalScalar<int>(healthcheckNode["timeout"]).value_or(30)
        };
    }

    ServiceDefinition ParseService(const std::string& tagName, const YAML::Node& serviceNode) {
        auto def = devkit::ServiceDefinition {
            .dependsOn = GetList<std::string>(serviceNode["dependsOn"]),
            .workingDirectory = OrWorkingDirectory(serviceNode["workingDirectory"]),
            .utf8 = GetBool(serviceNode["utf8"], true),
            .environment = GetMap(serviceNode["environment"]),
            .startCommand = GetShellCommandSequence(serviceNode["startCommand"]),
            .stopCommand = GetShellCommandSequence(serviceNode["stopCommand"]),
            .detachAfterSeconds = GetOptionalScalar<int>(serviceNode["detachAfterSeconds"]),
            .detachAfterMessage = GetOptionalScalar<std::string>(serviceNode["detachAfterMessage"]),
            .healthcheck = ParseHealthcheck(serviceNode["healthcheck"])
        };
        def.name = GetOptionalScalar<std::string>(serviceNode["name"]).value_or(tagName);

        auto monitorProcessNode = serviceNode["monitorProcess"];
        if (monitorProcessNode) {
            if (monitorProcessNode.IsScalar()) {
                def.monitorProcessFilter = ProcessFilter().WithExecutablePattern(monitorProcessNode.as<std::string>());
            } else if (monitorProcessNode.IsMap()) {
                auto processFilter = ProcessFilter();
                auto executableNode = monitorProcessNode["executable"];
                if (executableNode && executableNode.IsScalar()) {
                    processFilter.WithExecutablePattern(executableNode.as<std::string>());
                }
                auto argsNode = monitorProcessNode["args"];
                if (argsNode && argsNode.IsScalar()) {
                    processFilter.WithCommandLineArgsPattern(argsNode.as<std::string>());
                }
                auto wdNode = monitorProcessNode["workingDirectory"];
                if (wdNode && wdNode.IsScalar()) {
                    processFilter.WithWorkingDirectoryPattern(wdNode.as<std::string>());
                }
                def.monitorProcessFilter = processFilter;
            }
        }
        return def;
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
        std::filesystem::path workingDirectory = OrWorkingDirectory(node["workingDirectory"]);
        std::vector<std::string> dependsOn = GetList<std::string>(node["dependsOn"]);
        std::vector<std::string> before = GetList<std::string>(node["before"]);
        std::vector<std::string> after = GetList<std::string>(node["after"]);
        bool hidden = GetBool(node["hidden"], false);
        bool utf8 = GetBool(node["utf8"], true);
        int timeout = GetOptionalScalar<int>(node["timeout"]).value_or(-1);
        std::vector<int> exitCodes = GetList<int>(node["exitCodes"]);

        auto& commandNode = node["command"];
        if (!commandNode) {
            throw std::runtime_error("Could not parse task " + name + ": no command specified");
        }
        
        std::vector<std::string> commands = GetShellCommandSequence(commandNode);
        std::unordered_map<std::string, std::string> taskEnv = GetMap(node["environment"]);
        
        return std::make_shared<ShellCommandTask>(
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