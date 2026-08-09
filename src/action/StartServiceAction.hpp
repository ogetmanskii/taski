#pragma once
#include "Action.hpp"
#include "../context/PipelineContext.hpp"

namespace devkit {
    class StartServiceAction : public Action {
    public:
        StartServiceAction(std::shared_ptr<Service> service)
            : service(std::move(service)) {
        }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
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