#include "inja/inja.hpp"
#include "yaml-cpp/yaml.h"
#include "parsing.hpp"
#include "../logging/console_logging.hpp"
#include "../util/util.hpp"
#include "../services/services.hpp"
#include "inja_env.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>
#include <string>
#include <exception>

using namespace inja;

namespace devkit {

    static std::string RenderInjaTemplate(
        const std::string& templateString,
        const std::unordered_map<std::string, std::string>& env) {

        static Environment injaEnvironment = GetInjaEnvironment();

        return injaEnvironment.render(templateString, env);
    }

    static bool GetBool(const YAML::Node& node, bool defaultValue) {
        if (!node || !node.IsScalar()) {
            return defaultValue;
        }
        return node.as<bool>();
    }

    template <typename T> std::optional<T> GetOptional(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::nullopt;
        }
        return node.as<T>();
    }

    template <typename T> std::vector<T> GetList(const YAML::Node& node) {
        std::vector<T> list;
        if (!node) {
            return list;
        }
        if (node.IsSequence()) {
            for (int i = 0; i < node.size(); i++) {
                list.push_back(node[i].as<T>());
            }
        } else if (node.IsScalar()) {
            list.push_back(node.as<T>());
        }
        return list;
    }

    static std::unordered_map<std::string, std::string> GetMap(
        const YAML::Node& node,
        const std::unordered_map<std::string, std::string>& env) {

        std::unordered_map<std::string, std::string> map;
        if (!node || !node.IsMap()) {
            return map;
        }
        for (auto i = node.begin(); i != node.end(); i++) {
            map[i->first.as<std::string>()] = i->second.as<std::string>();
        }
        return map;
    }

    static std::optional<std::string> GetOptionalSingleLine(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::nullopt;
        }
        return TrimToSingleLine(node.as<std::string>());
    }

    static std::string GetWorkingDirectory(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::filesystem::current_path().string();
        }
        return node.as<std::string>();
    }

    static std::vector<std::string> ParseCommands(const YAML::Node& node) {
        std::vector<std::string> result;
        if (!node) {
            return result;
        }
        if (node.IsScalar()) {
            result.push_back(TrimToSingleLine(node.as<std::string>()));
        }
        if (node.IsSequence()) {
            for (auto it = node.begin(); it != node.end(); it++) {
                const std::string& value = it->as<std::string>();
                result.push_back(TrimToSingleLine(value));
            }
        }
        return result;
    }

    static devkit::ServiceDefinition ParseService(
        const std::string& tagName,
        const YAML::Node& serviceNode,
        const std::unordered_map<std::string, std::string>& env) {

        auto def = devkit::ServiceDefinition {
            .dependsOn = GetList<std::string>(serviceNode["dependsOn"]),
            .workingDirectory = GetWorkingDirectory(serviceNode["workingDirectory"]),
            .utf8 = GetBool(serviceNode["utf8"], true),
            .environment = GetMap(serviceNode["environment"], env),
            .startCommand = ParseCommands(serviceNode["startCommand"]),
            .stopCommand = ParseCommands(serviceNode["stopCommand"]),
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

    static void ParseServices(
        const YAML::Node& servicesNode,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices) {

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
                info("Could not parse service: {}", e.what());
            }
        }
    }

    static std::shared_ptr<Task> ParseTask(
        const std::string& tag,
        const YAML::Node& node,
        const std::unordered_map<std::string, std::string>& env) {

        std::string name = GetOptional<std::string>(node["name"]).value_or(tag);
        std::filesystem::path workingDirectory = GetWorkingDirectory(node["workingDirectory"]);
        std::vector<std::string> dependsOn = GetList<std::string>(node["dependsOn"]);
        std::vector<std::string> before = GetList<std::string>(node["before"]);
        std::vector<std::string> after = GetList<std::string>(node["after"]);
        bool hidden = GetBool(node["hidden"], false);
        bool utf8 = GetBool(node["utf8"], true);
        std::vector<int> exitCodes = GetList<int>(node["exitCodes"]);
        auto& commandNode = node["command"];
        if (commandNode) {
            std::vector<std::string> commands;
            if (commandNode.IsScalar()) {
                commands.push_back(commandNode.as<std::string>());
            } else if (commandNode.IsSequence()) {
                for (auto it = commandNode.begin(); it != commandNode.end(); it++) {
                    commands.push_back((*it).as<std::string>());
                }
            }
            std::unordered_map<std::string, std::string> taskEnv = GetMap(node["environment"], env);
            return std::make_shared<ShellCommandTask>(
                std::move(name),
                hidden,
                utf8,
                std::move(dependsOn),
                std::move(before),
                std::move(after),
                std::move(exitCodes),
                std::move(commands),
                std::move(workingDirectory),
                std::move(taskEnv)
            );
        }
        throw std::runtime_error("Could not parse task " + name + ": no command specified");
    }

    static void ParseTasks(
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
                info("Could not parse task: {}", e.what());
            }
        }
    }

    void ParseServicesYml(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks
    ) {
        std::string yaml = RenderInjaTemplate(devkit::ReadFileUtf8(filePath), env);

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