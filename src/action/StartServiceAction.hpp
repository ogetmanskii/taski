#pragma once

#include <string>
#include <utility>
#include <memory>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "action/Action.hpp"
#include "service/Service.hpp"

namespace devkit {
    class StartServiceAction : public Action {
    public:
        StartServiceAction(std::shared_ptr<Service> service)
            : service(std::move(service)) {
        }

        void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
            service->Start(pipeline.RefreshActiveProcesses());
        }

        std::string Description() override {
            return "Start " + service->definition.name;
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Service> service;
    };
}