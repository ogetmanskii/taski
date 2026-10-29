#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "service/Service.hpp"
#include "task/Task.hpp"

namespace devkit {

class ApplicationContext {
 private:
  const std::filesystem::path path;
  const std::unordered_map<std::string, std::string> env;

  const std::vector<std::unique_ptr<Service>> services;
  const std::vector<std::unique_ptr<Task>> tasks;

  const std::unordered_map<std::string, Service&> nameToService;
  const std::unordered_map<std::string, Task&> nameToTask;

 public:
  ApplicationContext(std::filesystem::path path,
                     std::unordered_map<std::string, std::string> env,
                     std::vector<std::unique_ptr<Service>> services,
                     std::vector<std::unique_ptr<Task>> tasks)
      : path(std::move(path)),
        env(std::move(env)),
        services(std::move(services)),
        tasks(std::move(tasks)),
        nameToService(BuildNameToServiceMap()),
        nameToTask(BuildNameToTaskMap()) {}

  const std::vector<std::unique_ptr<Service>>& GetServices() const { return services; }

  const std::vector<std::unique_ptr<Task>>& GetTasks() const { return tasks; }

  Service* GetService(const std::string& name) const {
    const auto it = nameToService.find(name);
    if (it == nameToService.end()) {
      return nullptr;
    }
    return &it->second;
  }

  Task* GetTask(const std::string& name) const {
    const auto it = nameToTask.find(name);
    if (it == nameToTask.end()) {
      return nullptr;
    }
    return &it->second;
  }

 private:
  std::unordered_map<std::string, Service&> BuildNameToServiceMap() {
    std::unordered_map<std::string, Service&> map;
    for (auto& service : services) {
      map.emplace(service->definition.name, *service);
    }
    return map;
  }

  std::unordered_map<std::string, Task&> BuildNameToTaskMap() {
    std::unordered_map<std::string, Task&> map;
    for (auto& task : tasks) {
      map.emplace(task->name, *task);
    }
    return map;
  }
};
}  // namespace devkit