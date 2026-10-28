#pragma once

#include <string>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "service/Service.hpp"

namespace devkit {

class StartServiceAction : public Action {
 public:
  StartServiceAction(Service& service) : service(service) {}

  void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
    service.Start(pipeline.RefreshActiveProcesses());
  }

  std::string Description() override { return "Start " + service.definition.name; }

  bool Counting() { return true; }

 private:
  Service& service;
};

}  // namespace devkit