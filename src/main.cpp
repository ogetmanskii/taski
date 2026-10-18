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
#include <map>

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

void PrintServices(std::shared_ptr<ApplicationContext> ctx) {
  if (ctx->GetServices().empty()) {
    return;
  }
  Info("SERVICES:");
  std::map<std::optional<std::string>, std::vector<std::shared_ptr<Service>>> serviceGroups;

  for (auto& service : ctx->GetServices()) {
    serviceGroups[service->definition.group].push_back(service);
  }

  for (auto& pair : serviceGroups) {
    auto& services = pair.second;
    if (pair.first) {
      // has group
      Info("  {}:", pair.first.value());
      for (auto& service : services) {
        auto& descriptionOpt = service->definition.description;
        if (descriptionOpt) {
          Info("    {} - {}", service->definition.name, Color::Yellow(descriptionOpt.value()));
        } else {
          Info("    {}", service->definition.name);
        }
      }
    } else {
      // has no group
      for (auto& service : services) {
        auto& descriptionOpt = service->definition.description;
        if (descriptionOpt) {
          Info("  {} - {}", service->definition.name, Color::Yellow(descriptionOpt.value()));
        } else {
          Info("  {}", service->definition.name);
        }
      }
    }
  }
}

void PrintTasks(std::shared_ptr<ApplicationContext> ctx) {
  if (ctx->GetTasks().empty()) {
    return;
  }
  Info("TASKS:");
  std::map<std::optional<std::string>, std::vector<std::shared_ptr<Task>>> taskGroups;

  for (auto& task : ctx->GetTasks()) {
    if (task->IsHidden()) {
      continue;
    }
    taskGroups[task->GetGroup()].push_back(task);
  }

  for (auto& pair : taskGroups) {
    auto& tasks = pair.second;
    if (pair.first) {
      // has group
      Info("  {}:", pair.first.value());
      for (auto& task : tasks) {
        auto& descriptionOpt = task->GetDescription();
        if (descriptionOpt) {
          Info("    {} - {}", task->GetName(), Color::Yellow(descriptionOpt.value()));
        } else {
          Info("    {}", task->GetName());
        }
      }
    } else {
      // has no group
      for (auto& task : tasks) {
        auto& descriptionOpt = task->GetDescription();
        if (descriptionOpt) {
          Info("  {} - {}", task->GetName(), Color::Yellow(descriptionOpt.value()));
        } else {
          Info("  {}", task->GetName());
        }
      }
    }
  }
}

void PrintServiceStatuses(std::shared_ptr<ApplicationContext> ctx) {
  if (ctx->GetServices().empty()) {
    Info("There is no defined services");
    return;
  }
  std::map<std::optional<std::string>, std::vector<std::shared_ptr<Service>>> serviceGroups;

  for (auto& service : ctx->GetServices()) {
    serviceGroups[service->definition.group].push_back(service);
  }

  auto activeProcesses = GetActiveProcesses();
  for (auto& pair : serviceGroups) {
    auto& services = pair.second;
    if (pair.first) {
      // has group
      Info("{}:", pair.first.value());
      for (auto& service : services) {
        auto status = service->Status(activeProcesses);
        const std::string& serviceName = service->definition.name;
        if (status == ServiceStatus::UP) {
          Console::Info("  {} {}", serviceName, Color::Green("on"));
        } else if (status == ServiceStatus::DOWN) {
          Console::Info("  {} {}", serviceName, Color::Gray("off"));
        } else {
          Console::Info("  {} {}", serviceName, "??");
        }
      }
    } else {
      // has no group
      for (auto& service : services) {
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
    }
  }
}

void ExecuteCommands(std::shared_ptr<ApplicationContext> ctx, const Args& args) {
  if (args.menu) {
    Menu::RunLoop(ctx);
  } else if (args.listCommand) {
    PrintServices(ctx);
    PrintTasks(ctx);
  } else if (args.printStatus) {
    PrintServiceStatuses(ctx);
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