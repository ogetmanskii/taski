#pragma once

#include <chrono>
#include <exception>
#include <format>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <memory>

#include "ApplicationContext.hpp"
#include "action/Action.hpp"
#include "console/Color.hpp"
#include "console/Console.hpp"
#include "console/DurationFormatter.hpp"
#include "processes/ProcessDescriptor.hpp"
#include "processes/Processes.hpp"

namespace devkit {

class Pipeline {
 public:
  Pipeline(ApplicationContext& appContext) : appContext(appContext) {}

  bool InsertKey(const std::string& key) { return keys.insert(key).second; }

  void PlanAction(std::unique_ptr<Action> action) { actions.push_back(std::move(action)); }

  void PlanPostAction(const std::string& key, std::unique_ptr<Action> action) {
    postActions.push_back(std::make_pair(key, std::move(action)));
  }

  std::vector<ProcessDescriptor>& ActiveProcesses() {
    if (!activeProcesses) {
      activeProcesses = GetActiveProcesses();
    }
    return *activeProcesses;
  }

  std::vector<ProcessDescriptor>& RefreshActiveProcesses() {
    activeProcesses = GetActiveProcesses();
    return *activeProcesses;
  }

  void Execute() {
    for (std::pair<std::string, std::unique_ptr<Action>>& postAction : postActions) {
      if (InsertKey(postAction.first)) {
        actions.push_back(std::move(postAction.second));
      }
    }
    if (actions.empty()) {
      return;
    }
    int total = 0;
    for (auto& action : actions) {
      if (action->Counting()) {
        total++;
      }
    }
    auto pipelineStart = std::chrono::high_resolution_clock::now();
    int i = 1;
    for (auto& action : actions) {
      if (action->Counting()) {
        Console::Info("-- [{}/{}] {}", i, total, action->Description());
        i++;
      } else {
        Console::Info("-- {}", action->Description());
      }
      try {
        action->Run(appContext, *this);
      } catch (const std::exception& e) {
        std::string message = std::format("-- {} - failed: {}", action->Description(), e.what());
        Console::Info(message);
        throw e;
      }
    }
    auto pipelineEnd = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
    Console::Info("-- {}: completed in {}", Color::Green("OK"), DurationFormatter::Format(totalMs));
  }

 private:
  std::optional<std::vector<ProcessDescriptor>> activeProcesses;

  ApplicationContext& appContext;
  std::vector<std::unique_ptr<Action>> actions;
  std::vector<std::pair<std::string /* key */, std::unique_ptr<Action>>> postActions;
  std::unordered_set<std::string> keys;
};
}  // namespace devkit