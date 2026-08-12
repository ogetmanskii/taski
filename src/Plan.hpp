#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include <ApplicationContext.hpp>
#include <action/Action.hpp>
#include <action/EnsureServiceisHealthyAction.hpp>
#include <action/StopServiceAction.hpp>
#include <action/StartServiceAction.hpp>
#include <action/RunTaskAction.hpp>

using namespace devkit;

namespace {

    void GetAllDependsOnServices(
        ApplicationContext& ctx,
        Service& service,
        std::vector<std::string>& outServiceNames,
        std::unordered_set<std::string>& observed) {

        if (!observed.insert(service.definition.name).second) {
            return;
        }
        for (const std::string& dependsOnServiceName : service.definition.dependsOn) {
            auto serviceOpt = ctx.GetService(dependsOnServiceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + dependsOnServiceName);
            }
            GetAllDependsOnServices(ctx, **serviceOpt, outServiceNames, observed);
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

    void GetAllDependsOnTasks(
        ApplicationContext& ctx,
        Task& task,
        std::vector<std::string>& outTaskNames,
        std::unordered_set<std::string>& observed) {

        if (!observed.insert(task.GetName()).second) {
            return;
        }
        for (const std::string& dependsOnTaskName : task.GetDependsOn()) {
            auto taskOpt = ctx.GetTask(dependsOnTaskName);
            if (!taskOpt) {
                throw std::runtime_error("No such task: " + dependsOnTaskName);
            }
            GetAllDependsOnTasks(ctx, **taskOpt, outTaskNames, observed);
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
}

namespace devkit::Plan {

    inline void PlanStartService(ApplicationContext& ctx, Pipeline& pipeline, std::shared_ptr<Service> service) {
        for (auto& dependsOnServiceName : GetAllDependsOnServices(ctx, *service)) {
            if (pipeline.InsertKey("ensure-healthy: " + dependsOnServiceName)) {
                pipeline.PlanAction(std::make_shared<EnsureServiceIsHealthyAction>(dependsOnServiceName, true));
            }
        }
        pipeline.PlanAction(std::make_shared<StartServiceAction>(service));
        auto& serviceDefinition = (*service).definition;
        if (serviceDefinition.healthcheck) {
            pipeline.PlanPostAction("ensure-healthy: " + serviceDefinition.name, std::make_shared<EnsureServiceIsHealthyAction>(serviceDefinition.name, false));
        }
    }

    inline void PlanStopService(ApplicationContext& ctx, Pipeline& pipeline, std::shared_ptr<Service> service) {
        pipeline.PlanAction(std::make_shared<StopServiceAction>(service));
    }

    inline void PlanTask(ApplicationContext& ctx, Pipeline& pipeline, std::shared_ptr<Task> task) {
        // Run before tasks
        for (auto& beforeTaskName : task->GetBefore()) {
            auto& beforeTask = ctx.GetTask(beforeTaskName);
            if (!beforeTask) {
                throw std::runtime_error("No such task: " + beforeTaskName);
            }
            PlanTask(ctx, pipeline, *beforeTask);
        }
        // Run dependsOn tasks
        for (auto& dependsOnTaskName : GetAllDependsOnTasks(ctx, *task)) {
            auto& dependsOnTask = ctx.GetTask(dependsOnTaskName);
            if (pipeline.InsertKey("run-task: " + dependsOnTaskName)) {
                pipeline.PlanAction(std::make_shared<RunTaskAction>(*dependsOnTask));
            }
        }
        // Run task itself
        pipeline.InsertKey("run-task: " + task->GetName());
        pipeline.PlanAction(std::make_shared<RunTaskAction>(task));
        // Run after tasks
        for (auto& afterTaskName : task->GetAfter()) {
            auto& afterTask = ctx.GetTask(afterTaskName);
            if (!afterTask) {
                throw std::runtime_error("No such task: " + afterTaskName);
            }
            PlanTask(ctx, pipeline, *afterTask);
        }
    }

    inline void PlanStartAllServices(ApplicationContext& ctx, Pipeline& pipeline) {
        for (auto& service : ctx.GetServices()) {
            PlanStartService(ctx, pipeline, service);
        }
    }

    inline void PlanStopAllServices(ApplicationContext& ctx, Pipeline& pipeline) {
        for (auto& service : ctx.GetServices()) {
            pipeline.PlanAction(std::make_shared<StopServiceAction>(service));
        }
    }
}