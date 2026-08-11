#pragma once

#include <exception>

#include <action/Action.hpp>
#include <Pipeline.hpp>

namespace devkit {

    class EnsureServiceIsHealthyAction : public Action {
    public:
        EnsureServiceIsHealthyAction(std::string serviceName)
            : serviceName(std::move(serviceName)) {
        }

        void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
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
            if (!service.WaitForHealthy()) {
                throw std::runtime_error(serviceName + " not healthy");
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