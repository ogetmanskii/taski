#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <util/StringUtils.hpp>

namespace devkit::YamlUtils {

    template <typename T> std::optional<T> GetOptionalScalar(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::nullopt;
        }
        return node.as<T>();
    }

    template <typename K, typename V> std::optional<std::unordered_map<K, V>> GetOptionalMap(const YAML::Node& node) {
        if (!node || !node.IsMap()) {
            return std::nullopt;
        }
        std::unordered_map<K, V> map;
        for (auto i = node.begin(); i != node.end(); i++) {
            map[i->first.as<K>()] = i->second.as<V>();
        }
        return map;
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

    template <typename T> T GetOrDefault(const YAML::Node& node, T defaultValue) {
        if (!node || !node.IsScalar()) {
            return defaultValue;
        }
        return node.as<T>();
    }

    inline std::unordered_map<std::string, std::string> GetMap(const YAML::Node& node) {
        std::unordered_map<std::string, std::string> map;
        if (!node || !node.IsMap()) {
            return map;
        }
        for (auto i = node.begin(); i != node.end(); i++) {
            map[i->first.as<std::string>()] = i->second.as<std::string>();
        }
        return map;
    }

    inline std::string OrWorkingDirectory(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::filesystem::current_path().string();
        }
        return node.as<std::string>();
    }

    inline std::vector<std::string> GetShellCommandSequence(const YAML::Node& node) {
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

    inline std::optional<std::string> GetSingleLineOpt(const YAML::Node& node) {
        if (!node || !node.IsScalar()) {
            return std::nullopt;
        }
        return StringUtils::TrimToSingleLine(node.as<std::string>());
    }
}