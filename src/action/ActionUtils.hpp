#pragma once
#include "Action.hpp"
#include "EnsureServiceisUpAction.hpp"
#include "StopServiceAction.hpp"
#include "StartServiceAction.hpp"
#include "RunTaskAction.hpp"

namespace devkit {
    std::vector<std::string> GetAllDependsOnServices(AppContext& ctx, Service& service);

    std::vector<std::string> GetAllDependsOnTasks(AppContext& ctx, Task& task);

    static void RegisterStartServiceAction(AppContext& ctx, PipelineContext& pipeline, std::shared_ptr<Service> service) {
        for (auto& dependsOnServiceName : GetAllDependsOnServices(ctx, *service)) {
            pipeline.AddAction(std::make_shared<EnsureServiceIsUpAction>(dependsOnServiceName));
        }
        pipeline.AddAction(std::make_shared<StartServiceAction>(service));
    }

    static void RegisterStopServiceAction(AppContext& ctx, PipelineContext& pipeline, std::shared_ptr<Service> service) {
        pipeline.AddAction(std::make_shared<StopServiceAction>(service));
    }

    static void RegisterRunTaskAction(AppContext& ctx, PipelineContext& pipeline, std::shared_ptr<Task> task) {
        // Run before tasks
        for (auto& beforeTaskName : task->GetBefore()) {
            auto& beforeTask = ctx.GetTask(beforeTaskName);
            if (!beforeTask) {
                throw std::runtime_error("No such task: " + beforeTaskName);
            }
            RegisterRunTaskAction(ctx, pipeline, *beforeTask);
        }
        // Run dependsOn tasks
        for (auto& dependsOnTaskName : GetAllDependsOnTasks(ctx, *task)) {
            auto& dependsOnTask = ctx.GetTask(dependsOnTaskName);
            if (pipeline.InsertKey("run-task: " + dependsOnTaskName)) {
                pipeline.AddAction(std::make_shared<RunTaskAction>(*dependsOnTask));
            }
        }
        // Run task iteself
        pipeline.InsertKey("run-task: " + task->GetName());
        pipeline.AddAction(std::make_shared<RunTaskAction>(task));
        // Run after tasks
        for (auto& afterTaskName : task->GetAfter()) {
            auto& afterTask = ctx.GetTask(afterTaskName);
            if (!afterTask) {
                throw std::runtime_error("No such task: " + afterTaskName);
            }
            RegisterRunTaskAction(ctx, pipeline, *afterTask);
        }
    }

    static void RegisterStartAllServicesAction(AppContext& ctx, PipelineContext& pipeline) {
        for (auto& service : ctx.GetServices()) {
            RegisterStartServiceAction(ctx, pipeline, service);
        }
    }

    static void RegisterStopAllServicesAction(AppContext& ctx, PipelineContext& pipeline) {
        for (auto& service : ctx.GetServices()) {
            pipeline.AddAction(std::make_shared<StopServiceAction>(service));
        }
    }
}