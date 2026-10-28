#pragma once

#include <string>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "service/Service.hpp"

namespace devkit {

class StopServiceAction : public Action {
 public:
  StopServiceAction(Service& service) : service(service) {}

  void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
    service.Stop(pipeline.RefreshActiveProcesses());
  }

  std::string Description() override { return "Stop " + service.definition.name; }

  bool Counting() { return true; }

 private:
  Service& service;
};

}  // namespace devkit