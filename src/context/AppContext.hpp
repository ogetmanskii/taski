#pragma once

#include "../service/Service.hpp"
#include "../task/Task.hpp"

#include <string>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <memory>

namespace devkit {

    namespace fs = std::filesystem;

    static inline std::unordered_map<std::string, std::shared_ptr<Service>> BuildNameToServiceMap(
        const std::vector<std::shared_ptr<Service>>& services) {
        std::unordered_map<std::string, std::shared_ptr<Service>> map;
        for (auto& service : services) {
            map[service->definition.name] = service;
        }
        return map;
    }

    static inline std::unordered_map<std::string, std::shared_ptr<Task>> BuildNameToTaskMap(
        const std::vector<std::shared_ptr<Task>>& tasks) {
        std::unordered_map<std::string, std::shared_ptr<Task>> map;
        for (auto& task : tasks) {
            map[task->GetName()] = task;
        }
        return map;
    }

    class AppContext {
    private:
        const fs::path path;
        const std::unordered_map<std::string, std::string> env;
        const std::unordered_map<std::string, std::shared_ptr<Service>> nameToService;
        const std::unordered_map<std::string, std::shared_ptr<Task>> nameToTask;
        const std::vector<std::shared_ptr<Service>> services;
        const std::vector<std::shared_ptr<Task>> tasks;
    public:
        AppContext(
            fs::path path,
            std::unordered_map<std::string, std::string> env,
            std::vector<std::shared_ptr<Service>> services,
            std::vector<std::shared_ptr<Task>> tasks
        ) : 
            nameToService(BuildNameToServiceMap(services)),
            nameToTask(BuildNameToTaskMap(tasks)),
            path(std::move(path)),
            env(std::move(env)),
            services(std::move(services)),
            tasks(std::move(tasks))
        { }

        const std::vector<std::shared_ptr<Service>>& GetServices() const {
            return services;
        }

        const std::vector<std::shared_ptr<Task>>& GetTasks() const {
            return tasks;
        }

        const std::optional<std::shared_ptr<Service>> GetService(const std::string& name) const {
            const auto& it = nameToService.find(name);
            if (it == nameToService.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        const std::optional<std::shared_ptr<Task>> GetTask(const std::string& name) const {
            const auto& it = nameToTask.find(name);
            if (it == nameToTask.end()) {
                return std::nullopt;
            }
            return it->second;
        }
    };


}