#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include <utility>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "action/CheckServiceAction.hpp"
#include "action/RunTaskAction.hpp"
#include "action/StartServiceAction.hpp"
#include "action/StopServiceAction.hpp"
#include "service/Service.hpp"
#include "task/Task.hpp"

namespace devkit::Plan {

namespace {

void GetAllDependsOnServices(ApplicationContext& ctx,
                             Service& service,
                             std::vector<std::string>& outServiceNames,
                             std::unordered_set<std::string>& observed) {
  if (!observed.insert(service.definition.name).second) {
    return;
  }
  for (const std::string& dependsOnServiceName : service.definition.dependsOn) {
    auto* service = ctx.GetService(dependsOnServiceName);
    if (!service) {
      throw std::runtime_error("No such service: " + dependsOnServiceName);
    }
    GetAllDependsOnServices(ctx, *service, outServiceNames, observed);
    if (std::find(outServiceNames.begin(), outServiceNames.end(), dependsOnServiceName) == outServiceNames.end()) {
      outServiceNames.push_back(dependsOnServiceName);
    }
  }
}

std::vector<std::string> GetAllDependsOnServices(ApplicationContext& ctx, Service& service) {
  std::vector<std::string> result;
  std::unordered_set<std::string> observed;
  GetAllDependsOnServices(ctx, service, result, observed);
  return result;
}

void GetAllDependsOnTasks(ApplicationContext& ctx,
                          Task& task,
                          std::vector<std::string>& outTaskNames,
                          std::unordered_set<std::string>& observed) {
  if (!observed.insert(task.name).second) {
    return;
  }
  for (const std::string& dependsOnTaskName : task.dependsOn) {
    auto* task = ctx.GetTask(dependsOnTaskName);
    if (!task) {
      throw std::runtime_error("No such task: " + dependsOnTaskName);
    }
    GetAllDependsOnTasks(ctx, *task, outTaskNames, observed);
    if (std::find(outTaskNames.begin(), outTaskNames.end(), dependsOnTaskName) == outTaskNames.end()) {
      outTaskNames.push_back(dependsOnTaskName);
    }
  }
}

std::vector<std::string> GetAllDependsOnTasks(ApplicationContext& ctx, Task& task) {
  std::vector<std::string> result;
  std::unordered_set<std::string> observed;
  GetAllDependsOnTasks(ctx, task, result, observed);
  return result;
}

}  // namespace

inline void PlanStartService(ApplicationContext& ctx, Pipeline& pipeline, Service& service) {
  for (auto& dependsOnServiceName : GetAllDependsOnServices(ctx, service)) {
    if (pipeline.InsertKey("ensure-healthy: " + dependsOnServiceName)) {
      pipeline.PlanAction(std::make_unique<CheckServiceAction>(dependsOnServiceName, true));
    }
  }
  pipeline.PlanAction(std::make_unique<StartServiceAction>(service));
  auto& serviceDefinition = service.definition;
  if (serviceDefinition.healthcheck) {
    pipeline.PlanPostAction("ensure-healthy: " + serviceDefinition.name,
                            std::make_unique<CheckServiceAction>(serviceDefinition.name, false));
  }
}

inline void PlanStopService(ApplicationContext& ctx, Pipeline& pipeline, Service& service) {
  pipeline.PlanAction(std::make_unique<StopServiceAction>(service));
}

inline void PlanTask(ApplicationContext& ctx, Pipeline& pipeline, Task& task) {
  // Run before tasks
  for (auto& beforeTaskName : task.before) {
    auto* beforTask = ctx.GetTask(beforeTaskName);
    if (!beforTask) {
      throw std::runtime_error("No such task: " + beforeTaskName);
    }
    PlanTask(ctx, pipeline, *beforTask);
  }
  // Run dependsOn tasks
  for (auto& dependsOnTaskName : GetAllDependsOnTasks(ctx, task)) {
    auto* dependsOnTask = ctx.GetTask(dependsOnTaskName);
    if (pipeline.InsertKey("run-task: " + dependsOnTaskName)) {
      pipeline.PlanAction(std::make_unique<RunTaskAction>(*dependsOnTask));
    }
  }
  // Run task itself
  pipeline.InsertKey("run-task: " + task.name);
  pipeline.PlanAction(std::make_unique<RunTaskAction>(task));
  // Run after tasks
  for (auto& afterTaskName : task.after) {
    auto* afterTask = ctx.GetTask(afterTaskName);
    if (!afterTask) {
      throw std::runtime_error("No such task: " + afterTaskName);
    }
    PlanTask(ctx, pipeline, *afterTask);
  }
}

inline void PlanStartAllServices(ApplicationContext& ctx, Pipeline& pipeline) {
  for (auto& service : ctx.GetServices()) {
    PlanStartService(ctx, pipeline, *service);
  }
}

inline void PlanStopAllServices(ApplicationContext& ctx, Pipeline& pipeline) {
  // Остановка всех сервисов - в порядке, обратном запуску всех, с учетом зависимостей
  std::vector<std::unique_ptr<Action>> actions;
  actions.reserve(ctx.GetServices().size());

  for (auto& servicePtr : ctx.GetServices()) {
    Service& service = *servicePtr;
    for (auto& dependsOnServiceName : GetAllDependsOnServices(ctx, service)) {
      std::optional<Service*> dependsOnServiceOpt = ctx.GetService(dependsOnServiceName);
      if (dependsOnServiceOpt && pipeline.InsertKey("stop: " + dependsOnServiceName)) {
        actions.push_back(std::make_unique<StopServiceAction>(**dependsOnServiceOpt));
      }
    }
    if (pipeline.InsertKey("stop: " + service.definition.name)) {
      actions.push_back(std::make_unique<StopServiceAction>(*servicePtr));
    }
  }

  for (auto it = actions.rbegin(); it != actions.rend(); it++) {
    pipeline.PlanAction(std::move(*it));
  }
}
}  // namespace devkit::Plan