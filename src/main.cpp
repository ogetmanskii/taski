#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "Plan.hpp"
#include "args/args.hpp"
#include "console/Color.hpp"
#include "console/Console.hpp"
#include "menu/MenuLoop.hpp"
#include "parser/DotEnvParser.hpp"
#include "parser/EnvironmentParser.hpp"
#include "processes/Processes.hpp"
#include "service/Service.hpp"
#include "task/Task.hpp"
#include "version.h"

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
  Plan::PlanStartAllServices(*ctx, pipeline);
  pipeline.Execute();
}

void StopAllServices(std::shared_ptr<ApplicationContext> ctx) {
  Pipeline pipeline(ctx);
  Plan::PlanStopAllServices(*ctx, pipeline);
  pipeline.Execute();
}

void ExecuteCommands(std::shared_ptr<ApplicationContext> ctx, const Args& args) {
  if (args.menu) {
    Menu::RunLoop(ctx);
  } else if (args.listCommand) {
    if (!ctx->GetServices().empty()) {
      Info("SERVICES:");
      for (auto& service : ctx->GetServices()) {
        Info("  {}", service->definition.name);
      }
    }
    if (!ctx->GetTasks().empty()) {
      Info("TASKS:");
      for (auto& task : ctx->GetTasks()) {
        if (task->IsHidden()) {
          continue;
        }
        Info("  {}", task->GetName());
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
}  // namespace

int main(int argc, char* argv[]) {
  Console::Init();
  Args args = Args::FromArgv(argc, argv);
  if (args.exitCode.has_value()) {
    // Exit with error
    return *args.exitCode;
  }

  if (args.noColor) {
    Console::Color::SetNoColorMode();
  }

  // Print version
  if (args.versionCommand) {
    Console::Info(Version::GetVersion());
    return 0;
  }

  // Override current path
  fs::path currentPath = args.currentPath;
  if (!currentPath.is_absolute()) {
    currentPath = fs::current_path() / currentPath;
  }
  fs::current_path(currentPath);

  // Override env
  std::unordered_map<std::string, std::string> env;
  fs::path dotEnvFile = currentPath / args.dotEnvFile;
  try {
    Parser::ParseDotEnvFile(dotEnvFile, env);
  } catch (const std::exception& e) {
    Console::Info("Could not parse .env file " + dotEnvFile.string() + ": " + e.what());
    return 1;
  }
  for (auto& it : env) {
    PutEnv(it);
  }

  // Setup context
  std::vector<std::shared_ptr<Service>> services;
  std::vector<std::shared_ptr<Task>> tasks;
  fs::path environmentFile = currentPath / args.environmentFile;
  try {
    Parser::ParseEnvironmentYmlFile(environmentFile, env, services, tasks);
  } catch (const std::exception& e) {
    Console::Info("Could not parse environment file " + environmentFile.string() + ": " + e.what());
    return 1;
  }

  std::shared_ptr<ApplicationContext> context = std::make_shared<ApplicationContext>(
      std::move(currentPath), std::move(env), std::move(services), std::move(tasks));

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