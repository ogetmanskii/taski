#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

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

    devkit::ServiceDefinition ParseService(
        const std::string& tagName,
        const YAML::Node& serviceNode,
        const std::unordered_map<std::string, std::string>& env) {

        auto def = devkit::ServiceDefinition {
            .dependsOn = GetList<std::string>(serviceNode["dependsOn"]),
            .workingDirectory = OrWorkingDirectory(serviceNode["workingDirectory"]),
            .utf8 = GetBool(serviceNode["utf8"], true),
            .environment = GetMap(serviceNode["environment"], env),
            .startCommand = GetShellCommandSequence(serviceNode["startCommand"]),
            .stopCommand = GetShellCommandSequence(serviceNode["stopCommand"]),
            .detachAfterSeconds = GetOptional<int>(serviceNode["detachAfterSeconds"]),
            .detachAfterMessage = GetOptional<std::string>(serviceNode["detachAfterMessage"])
        };
        def.name = GetOptional<std::string>(serviceNode["name"]).value_or(tagName);

        auto monitorProcessNode = serviceNode["monitorProcess"];
        if (monitorProcessNode) {
            if (monitorProcessNode.IsScalar()) {
                def.monitorProcess = monitorProcessNode.as<std::string>();
            } else if (monitorProcessNode.IsMap()) {
                auto executableNode = monitorProcessNode["executable"];
                if (executableNode && executableNode.IsScalar()) {
                    def.monitorProcess = executableNode.as<std::string>();
                }
                auto argsNode = monitorProcessNode["args"];
                if (argsNode && argsNode.IsScalar()) {
                    def.monitorProcessArgsPattern = argsNode.as<std::string>();
                }
            }
        }
        return def;
    }

    void ParseServices(
        const YAML::Node& servicesNode,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<devkit::Service>>& outServices) {

        for (auto it = servicesNode.begin(); it != servicesNode.end(); it++) {
            const std::string& serviceTag = it->first.as<std::string>();
            const YAML::Node& serviceNode = it->second;
            if (!serviceNode.IsMap()) {
                continue;
            }
            try {
                auto definition = ParseService(serviceTag, serviceNode, env);
                outServices.push_back(std::make_shared<Service>(std::move(definition)));
            } catch (const std::exception& e) {
                Info("Could not parse service: {}", e.what());
            }
        }
    }

    std::shared_ptr<Task> ParseTask(
        const std::string& tag,
        const YAML::Node& node,
        const std::unordered_map<std::string, std::string>& env) {

        std::string name = GetOptional<std::string>(node["name"]).value_or(tag);
        std::filesystem::path workingDirectory = OrWorkingDirectory(node["workingDirectory"]);
        std::vector<std::string> dependsOn = GetList<std::string>(node["dependsOn"]);
        std::vector<std::string> before = GetList<std::string>(node["before"]);
        std::vector<std::string> after = GetList<std::string>(node["after"]);
        bool hidden = GetBool(node["hidden"], false);
        bool utf8 = GetBool(node["utf8"], true);
        int timeout = GetOptional<int>(node["timeout"]).value_or(-1);
        std::vector<int> exitCodes = GetList<int>(node["exitCodes"]);
        auto& commandNode = node["command"];
        if (!commandNode) {
            throw std::runtime_error("Could not parse task " + name + ": no command specified");
        }
        std::vector<std::string> commands = GetShellCommandSequence(commandNode);
        std::unordered_map<std::string, std::string> taskEnv = GetMap(node["environment"], env);
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

    void ParseTasks(
        const YAML::Node& tasksNode,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Task>>& outTasks) {

        for (auto it = tasksNode.begin(); it != tasksNode.end(); it++) {
            const std::string& taskTag = it->first.as<std::string>();
            const YAML::Node& taskNode = it->second;
            if (!taskNode.IsMap()) {
                continue;
            }
            try {
                outTasks.push_back(ParseTask(taskTag, taskNode, env));
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
            ParseServices(servicesNode, env, outServices);
        }

        YAML::Node tasksNode = rootNode["tasks"];
        if (tasksNode.IsMap()) {
            ParseTasks(tasksNode, env, outTasks);
        }
    }
}