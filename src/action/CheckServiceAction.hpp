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
    Service* service = appCtx.GetService(serviceName);
    if (!service) {
      throw std::runtime_error("No such service: " + serviceName);
    }
    if (autoStart) {
      auto status = service->Status(pipeline.RefreshActiveProcesses());
      if (status == ServiceStatus::DOWN) {
        Info("-- Start {}", serviceName);
        service->Start(pipeline.ActiveProcesses());
      }
    }
    if (!service->WaitForHealthy()) {
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