#pragma once

#include <string>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "task/Task.hpp"

namespace devkit {
class RunTaskAction : public Action {
 public:
  RunTaskAction(Task& task) : task(task) {}

  void Run(ApplicationContext& appCtx, Pipeline& pipeline) override { task.Run(); }

  std::string Description() override { return task.name; }

  bool Counting() { return true; }

 private:
  Task& task;
};
}  // namespace devkit