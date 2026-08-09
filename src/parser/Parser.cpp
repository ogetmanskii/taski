#include "Parser.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "inja/inja.hpp"
#include "yaml-cpp/yaml.h"

#include "../service/Service.hpp"
#include "../service/ServiceDefinition.hpp"
#include "../util/StringUtils.hpp"
#include "../util/FileUtils.hpp"
#include "../task/ShellCommandTask.hpp"

namespace {

    using namespace inja;
    using namespace devkit;

    inja::Environment GetInjaEnvironment() {
        inja::Environment injaEnvironment;
        injaEnvironment.set_expression("${", "}");
        injaEnvironment.set_html_autoescape(false);
        return injaEnvironment;
    }

    std::string TrimWhitespace(const std::string& str) {
        std::size_t first = str.find_first_not_of(" \t");
        std::size_t last = str.find_last_not_of(" \t");
        if (first == std::string::npos) {
            return "";
        }
        return str.substr(first, last - first + 1);
    }

    std::string RenderInjaTemplate(
        const std::string& templateString,
        const std::unordered_map<std::string, std::string>& env) {

        static Environment injaEnvironment = GetInjaEnvironment();

        return injaEnvironment.render(templateString, env);
    }

    bool GetBool(const YAML::Node& node, bool defaultValue) {
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

    std::unordered_map<std::string, std::string> GetMap(
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

    std::optional<std::string> GetOptionalSingleLine(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::nullopt;
        }
        return TrimToSingleLine(node.as<std::string>());
    }

    std::string GetWorkingDirectory(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::filesystem::current_path().string();
        }
        return node.as<std::string>();
    }

    std::vector<std::string> ParseCommands(const YAML::Node& node) {
        std::vector<std::string> result;
        if (!node) {
            return result;
        }
        if (node.IsScalar()) {
            result.push_back(StringUtils::TrimToSingleLine(node.as<std::string>()));
        }
        if (node.IsSequence()) {
            for (auto it = node.begin(); it != node.end(); it++) {
                const std::string& value = it->as<std::string>();
                result.push_back(StringUtils::TrimToSingleLine(value));
            }
        }
        return result;
    }

    devkit::ServiceDefinition ParseService(
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

    std::unordered_map<std::string, std::string> ParseDotEnvFile(const std::string& filename) {
        static inja::Environment injaEnv = GetInjaEnvironment();

        std::unordered_map<std::string, std::string> data;

        std::ifstream file(filename);
        if (!file.is_open()) {
            return data;
        }

        nlohmann::json jsonData;
        std::string line;
        while (std::getline(file, line)) {
            // skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            // parse key-value pairs
            auto delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) {
                throw std::runtime_error("Invalid line format: " + line);
            }
            std::string key = TrimWhitespace(line.substr(0, delimiterPos));
            std::string value = TrimWhitespace(line.substr(delimiterPos + 1));

            data[key] = injaEnv.render(value, jsonData);
            jsonData[key] = data[key];
        }
        return data;
    }

    void ParseEnvironmentYamlFile(
        const std::filesystem::path& filePath,
        const std::unordered_map<std::string, std::string>& env,
        std::vector<std::shared_ptr<Service>>& outServices,
        std::vector<std::shared_ptr<Task>>& outTasks
        ) {
        std::string yaml = RenderInjaTemplate(FileUtils::ReadFileUtf8(filePath), env);

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