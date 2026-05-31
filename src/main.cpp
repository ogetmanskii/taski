#include "logging/console_logging.hpp"
#include "menu/menu.hpp"
#include "processes/processes.hpp"
#include "parsing/parsing.hpp"
#include "util/util.hpp"
#include "tasks/tasks.hpp"
#include "parsing/env_parser.hpp"
#include "args/args.hpp"
#include "action/action.hpp"
#include "context/app_context.hpp"
#include <optional>
#include "stdlib.h"
#include "conio.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>
#include <memory>
#include "../build/generated/version.h"

using namespace devkit;
namespace fs = std::filesystem;

static std::unordered_map<std::string, std::string> GetEnv(const fs::path& currentPath, const std::string& dotEnvFile) {
    
    try {
        EnvParser envParser;
        std::unordered_map<std::string, std::string> env = envParser.ParseFromFile((currentPath / dotEnvFile).string());
        for (auto it = env.begin(); it != env.end(); it++) {
            const auto& pair = *it;
            std::string envString = pair.first + "=" + pair.second;
            if (putenv(envString.c_str()) != 0) {
                info("Warning: could not set env string: {}", envString);
            }
        }
        return env;
    } catch (const std::exception& e) {
        info("Could not parse .env file: {}", e.what());
        return std::unordered_map<std::string, std::string>();
    }
}

static void ParseEnvironment(
    const fs::path& currentPath, 
    const std::string& environmentFile,
    const std::unordered_map<std::string, std::string>& env,
    std::vector<std::shared_ptr<Service>>& outServices,
    std::vector<std::shared_ptr<Task>>& outTasks) {

    try {
        ParseServicesYml(currentPath / environmentFile, env, outServices, outTasks);
    } catch (const std::exception& e) {
        info("Could not parse services: {}", e.what());
    }
}

static void StartAllServices(std::shared_ptr<AppContext> ctx) {
    PipelineContext pipeline(ctx);
    for (auto& service : ctx->GetServices()) {
        pipeline.AddAction(std::make_shared<StartServiceAction>(service));
    }
    pipeline.Run();
}

static void StopAllServices(std::shared_ptr<AppContext> ctx) {
    PipelineContext pipeline(ctx);
    for (auto& service : ctx->GetServices()) {
        pipeline.AddAction(std::make_shared<StopServiceAction>(service));
    }
    pipeline.Run();
}

static constexpr char ENTER = '\r';
static constexpr char ESCAPE = 27;

static inline bool IsReturn(const char c) {
    return c == ENTER || c == ESCAPE;
}

static void RunUserMenu(std::shared_ptr<AppContext> context) {
    while (true) {
        auto activeProcesses = GetActiveProcesses();
        std::vector<MenuItem> menu;
        if (context->GetServices().size() > 1) {
            menu.push_back({
                .title = "Start All",
                .description = "Start All Services",
                .shortcut = std::nullopt,
                .pipelineAction = RegisterStartAllServicesAction
            });
            menu.push_back({
                .title = "Stop All",
                .description = "Stop All Services",
                .shortcut = std::nullopt,
                .pipelineAction = RegisterStopAllServicesAction
            });
        }
        for (auto& service : context->GetServices()) {
            auto status = service->Status(activeProcesses);
            std::string serviceName = PadLeft(service->definition.name, context->GetServiceNameMaxLength());
            if (status == ServiceStatus::UP) {
                menu.push_back({
                    std::format("{} {}", serviceName, color::green("UP")),
                    std::format("Stop {}", service->definition.name),
                    std::nullopt,
                    [&](AppContext& ctx, PipelineContext& pipeline) {
                        RegisterStopServiceAction(ctx, pipeline, service);
                    }
                });
            } else if (status == ServiceStatus::DOWN) {
                menu.push_back({
                    std::format("{} {}", serviceName, color::red("DOWN")),
                    std::format("Start {}", service->definition.name),
                    std::nullopt,
                    [&](AppContext& ctx, PipelineContext& pipeline) {
                        RegisterStartServiceAction(ctx, pipeline, service);
                    }
                });
            } else {
                menu.push_back({
                    std::format("{}", serviceName),
                    std::format("Run {}", service->definition.name),
                    std::nullopt,
                    [&](AppContext& ctx, PipelineContext& pipeline) {
                        RegisterStartServiceAction(ctx, pipeline, service);
                    }
                });
            }
        }
        for (auto& task : context->GetTasks()) {
            if (task->IsHidden()) {
                continue;
            }
            menu.push_back({
                task->GetName(),
                task->GetName(),
                std::nullopt,
                [&](AppContext& ctx, PipelineContext& pipeline) {
                    RegisterRunTaskAction(ctx, pipeline, task);
                }
            });
        }
        if (devkit::ShowMenu(menu, context)) {
            while (!IsReturn(_getch())) {

            }
        }
        ClearScreen();
    }
}

static void ExecuteCommands(std::shared_ptr<AppContext> ctx, const Args& args) {
    if (args.menu) {
        RunUserMenu(ctx);
    } else if (args.listCommand) {
        if (!ctx->GetServices().empty()) {
            info("-- Services --");
            for (auto& service : ctx->GetServices()) {
                info("{}", service->definition.name);
            }
        }
        if (!ctx->GetTasks().empty()) {
            info("-- Tasks --");
            for (auto& task : ctx->GetTasks()) {
                if (task->IsHidden()) {
                    continue;
                }
                info("{}", task->GetName());
            }
        }
    } else if (args.printStatus) {
        auto activeProcesses = GetActiveProcesses();
        for (auto& service : ctx->GetServices()) {
            auto status = service->Status(activeProcesses);
            std::string serviceName = PadLeft(service->definition.name, ctx->GetServiceNameMaxLength());
            if (status == ServiceStatus::UP) {
                info("{} {}", serviceName, color::green("UP"));
            } else if (status == ServiceStatus::DOWN) {
                info("{} {}", serviceName, color::red("DOWN"));
            } else {
                info("{} {}", serviceName, "??");
            }
        }
    } else if (args.upCommand) {
        if (args.upServicesList.empty()) {
            StartAllServices(ctx);
        } else {
            PipelineContext pipeline(ctx);
            for (auto& name : args.upServicesList) {
                auto service = ctx->GetService(name);
                if (!service) {
                    throw std::runtime_error("No such service: " + name);
                }
                RegisterStartServiceAction(*ctx, pipeline, *service);
            }
            pipeline.Run();
        }
    } else if (args.downCommand) {
        if (args.downServicesList.empty()) {
            StopAllServices(ctx);
        } else {
            PipelineContext pipeline(ctx);
            for (auto& name : args.downServicesList) {
                auto service = ctx->GetService(name);
                if (!service) {
                    throw std::runtime_error("No such service: " + name);
                }
                RegisterStopServiceAction(*ctx, pipeline, *service);
            }
            pipeline.Run();
        }
    } else {
        PipelineContext pipeline(ctx);
        for (const auto& taskName : args.runList) {
            auto taskOpt = ctx->GetTask(taskName);
            if (!taskOpt) {
                throw std::runtime_error("No such task: " + taskName);
            }
            std::shared_ptr<Task> task = *taskOpt;
            if (task->IsHidden()) {
                throw std::runtime_error("Can not run hidden task: " + taskName);
            }
            RegisterRunTaskAction(*ctx, pipeline, task);
        }
        pipeline.Run();
    }
}

int main(int argc, char* argv[]) {
    InitConsole();
    Args args = GetArgs(argc, argv);
    if (args.exitCode.has_value()) {
        return *args.exitCode;
    }

    if (args.versionCommand) {
        info(Version::GetVersion());
        return 0;
    }

    fs::path path = args.currentPath;
    if (!path.is_absolute()) {
        path = fs::current_path() / path;
    }
    fs::current_path(path);

    std::unordered_map<std::string, std::string> env = GetEnv(path, args.dotEnvFile);

    std::vector<std::shared_ptr<Service>> services;
    std::vector<std::shared_ptr<Task>> tasks;
    ParseEnvironment(path, args.environmentFile, env, services, tasks);

    std::shared_ptr<AppContext> context = std::make_shared<AppContext>(
        std::move(path),
        std::move(env),
        std::move(services),
        std::move(tasks)
    );

    if (args.HasSpecificCommands()) {
        try {
            ExecuteCommands(context, args);
        } catch (const std::exception& e) {
            info("Error: {}", e.what());
            return 1;
        }
    } else {
        RunUserMenu(context);
    }

    return 0;
}