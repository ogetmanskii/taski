#pragma once
#include "Action.hpp"

namespace devkit {
    class EnsureServiceIsUpAction : public Action {
    public:
        EnsureServiceIsUpAction(std::string serviceName)
            : serviceName(std::move(serviceName)) {
        }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            auto& serviceOpt = appCtx.GetService(serviceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + serviceName);
            }
            auto service = **serviceOpt;
            auto status = service.Status(pipeline.RefreshActiveProcesses());
            if (status == ServiceStatus::DOWN) {
                Info("-- {} is down. Start it", serviceName);
                service.Start(pipeline.ActiveProcesses());
            } else if (status == ServiceStatus::UNKNOWN) {
                Info("-- {} is in unknown state. Do nothing");
            }
        }

        std::string Description() override {
            return "Ensure " + serviceName + " is up";
        }

        bool Counting() {
            return false;
        }

    private:
        std::string serviceName;
    };
}