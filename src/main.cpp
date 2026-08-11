#include <optional>
#include <iostream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include <unordered_map>
#include <exception>

#include <generated/version.h>

#include <Plan.hpp>
#include <ApplicationContext.hpp>
#include <Processes.hpp>

#include <console/Console.hpp>
#include <console/Color.hpp>
#include <parser/EnvironmentParser.hpp>
#include <parser/DotEnvParser.hpp>
#include <args/Args.hpp>
#include <task/Task.hpp>
#include <action/Action.hpp>
#include <action/RunTaskAction.hpp>
#include <action/StartServiceAction.hpp>
#include <action/StopServiceAction.hpp>
#include <menu/MenuLoop.hpp>

using namespace devkit;

namespace fs = std::filesystem;

namespace {

    void PutEnv(const std::pair<std::string, std::string>& envPair) {
        std::string envString = envPair.first + "=" + envPair.second;
        if (putenv(envString.c_str()) != 0) {
            Console::Info("Warning: could not set env string: {}", envString);
        }
    }

    void StartAllServices(std::shared_ptr<ApplicationContext> ctx) {
        Pipeline pipeline(ctx);
        for (auto& service : ctx->GetServices()) {
            pipeline.Plan(std::make_shared<StartServiceAction>(service));
        }
        pipeline.Execute();
    }

    void StopAllServices(std::shared_ptr<ApplicationContext> ctx) {
        Pipeline pipeline(ctx);
        for (auto& service : ctx->GetServices()) {
            pipeline.Plan(std::make_shared<StopServiceAction>(service));
        }
        pipeline.Execute();
    }

    void ExecuteCommands(std::shared_ptr<ApplicationContext> ctx, const Args& args) {
        if (args.menu) {
            Menu::RunLoop(ctx);
        } else if (args.listCommand) {
            if (!ctx->GetServices().empty()) {
                Info("-- Services --");
                for (auto& service : ctx->GetServices()) {
                    Info("{}", service->definition.name);
                }
            }
            if (!ctx->GetTasks().empty()) {
                Info("-- Tasks --");
                for (auto& task : ctx->GetTasks()) {
                    if (task->IsHidden()) {
                        continue;
                    }
                    Info("{}", task->GetName());
                }
            }
        } else if (args.printStatus) {
            auto activeProcesses = GetActiveProcesses();
            for (auto& service : ctx->GetServices()) {
                auto status = service->Status(activeProcesses);
                const std::string& serviceName = service->definition.name;
                if (status == ServiceStatus::UP) {
                    Console::Info("{} {}", serviceName, Color::Green("on"));
                } else if (status == ServiceStatus::DOWN) {
                    Console::Info("{} {}", serviceName, Color::Gray("off"));
                } else {
                    Console::Info("{} {}", serviceName, "??");
                }
            }
        } else if (args.upCommand) {
            if (args.upServicesList.empty()) {
                StartAllServices(ctx);
            } else {
                Pipeline pipeline(ctx);
                for (auto& name : args.upServicesList) {
                    auto service = ctx->GetService(name);
                    if (!service) {
                        throw std::runtime_error("No such service: " + name);
                    }
                    Plan::PlanStartService(*ctx, pipeline, *service);
                }
                pipeline.Execute();
            }
        } else if (args.downCommand) {
            if (args.downServicesList.empty()) {
                StopAllServices(ctx);
            } else {
                Pipeline pipeline(ctx);
                for (auto& name : args.downServicesList) {
                    auto service = ctx->GetService(name);
                    if (!service) {
                        throw std::runtime_error("No such service: " + name);
                    }
                    Plan::PlanStopService(*ctx, pipeline, *service);
                }
                pipeline.Execute();
            }
        } else if (!args.runList.empty()) {
            Pipeline pipeline(ctx);
            for (const auto& taskName : args.runList) {
                auto taskOpt = ctx->GetTask(taskName);
                if (!taskOpt) {
                    throw std::runtime_error("No such task: " + taskName);
                }
                std::shared_ptr<Task> task = *taskOpt;
                if (task->IsHidden()) {
                    throw std::runtime_error("Can not run hidden task: " + taskName);
                }
                Plan::PlanTask(*ctx, pipeline, task);
            }
            pipeline.Execute();
        }
    }
}

int main(int argc, char* argv[]) {
    Console::Init();
    Args args = Args::FromArgv(argc, argv);
    if (args.exitCode.has_value()) {
        // Exit with error
        return *args.exitCode;
    }

    // Print version
    if (args.versionCommand) {
        Console::Info(Version::GetVersion());
        return 0;
    }

    // Override current path
    fs::path path = args.currentPath;
    if (!path.is_absolute()) {
        path = fs::current_path() / path;
    }
    fs::current_path(path);

    // Override env
    std::unordered_map<std::string, std::string> env = Parser::ParseDotEnvFile(path / args.dotEnvFile);
    for (auto& it : env) {
        PutEnv(it);
    }

    // Setup context
    std::vector<std::shared_ptr<Service>> services;
    std::vector<std::shared_ptr<Task>> tasks;
    Parser::ParseEnvironmentYmlFile(path / args.environmentFile, env, services, tasks);

    std::shared_ptr<ApplicationContext> context = std::make_shared<ApplicationContext>(
        std::move(path),
        std::move(env),
        std::move(services),
        std::move(tasks)
    );

    // Run
    if (args.HasSpecificCommands()) {
        try {
            ExecuteCommands(context, args);
        } catch (const std::exception& e) {
            Console::Info("Error: {}", e.what());
            return 1;
        }
    } else {
        Menu::RunLoop(context);
    }

    return 0;
}