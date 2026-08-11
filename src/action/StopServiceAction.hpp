#pragma once

#include <string>
#include <memory>

#include <action/Action.hpp>

namespace devkit {
    class StopServiceAction : public Action {
    public:
        StopServiceAction(std::shared_ptr<Service> service)
            : service(std::move(service)) {
        }

        void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
            service->Stop(pipeline.RefreshActiveProcesses());
        }

        std::string Description() override {
            return "Stop " + service->definition.name;
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Service> service;
    };
}