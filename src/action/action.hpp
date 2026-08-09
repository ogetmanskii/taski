#pragma once

#include "../context/app_context.hpp"
#include "../context/pipeline_context.hpp"
#include "../services/services.hpp"
#include "../tasks/tasks.hpp"
#include "../util/util.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <algorithm>
#include <unordered_set>

namespace devkit {

    class Action  {
    public:
        virtual ~Action() = default;
        virtual void Run(AppContext& appCtx, PipelineContext& pipeline) = 0;
        virtual std::string Description() = 0;
        virtual bool Counting() = 0;
    };

    class StartServiceAction : public Action {
    public:
        StartServiceAction(std::shared_ptr<Service> service) 
            : service(std::move(service))
        { }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            service->Start(pipeline.RefreshActiveProcesses());
        }

        std::string Description() override {
            return "Start " + service->definition.name;
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Service> service;
    };

    class StopServiceAction : public Action {
    public:
        StopServiceAction(std::shared_ptr<Service> service)
            : service(std::move(service)) 
        { }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            service->Stop(pipeline.RefreshActiveProcesses());
        }

        std::string Description() override {
            return "Stop " + service->definition.name;
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Service> service;
    };

    class RunTaskAction : public Action {
    public:
        RunTaskAction(std::shared_ptr<Task> task)
            : task(std::move(task)) 
        { }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            task->Run();
        }

        std::string Description() override {
            return task->GetName();
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Task> task;
    };

    class EnsureServiceIsUpAction : public Action {
    public:
        EnsureServiceIsUpAction(std::string serviceName)
            : serviceName(std::move(serviceName)) {
        }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            auto& serviceOpt = appCtx.GetService(serviceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + serviceName);
            }
            auto service = **serviceOpt;
            auto status = service.Status(pipeline.RefreshActiveProcesses());
            if (status == ServiceStatus::DOWN) {
                info("-- {} is down. Start it", serviceName);
                service.Start(pipeline.ActiveProcesses());
            } else if (status == ServiceStatus::UNKNOWN) {
                info("-- {} is in unknown state. Do nothing");
            }
        }

        std::string Description() override {
            return "Ensure " + serviceName + " is up";
        }

        bool Counting() {
            return false;
        }

    private:
        std::string serviceName;
    };

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