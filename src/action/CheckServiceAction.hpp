#pragma once

#include <stdexcept>
#include <string>
#include <utility>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "console/Console.hpp"
#include "service/Service.hpp"

namespace devkit {

class CheckServiceAction : public Action {
 public:
  CheckServiceAction(std::string serviceName, bool autoStart)
      : serviceName(std::move(serviceName)), autoStart(autoStart) {}

  void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
    auto& serviceOpt = appCtx.GetService(serviceName);
    if (!serviceOpt) {
      throw std::runtime_error("No such service: " + serviceName);
    }
    auto& service = **serviceOpt;
    if (autoStart) {
      auto status = service.Status(pipeline.RefreshActiveProcesses());
      if (status == ServiceStatus::DOWN) {
        Info("-- Start {}", serviceName);
        service.Start(pipeline.ActiveProcesses());
      }
    }
    if (!service.WaitForHealthy()) {
      throw std::runtime_error(serviceName + " not healthy");
    }
  }

  std::string Description() override { return "Healthcheck " + serviceName; }

  bool Counting() { return false; }

 private:
  std::string serviceName;
  bool autoStart;
};
}  // namespace devkit