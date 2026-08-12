#pragma once

#include <exception>

#include <action/Action.hpp>
#include <Pipeline.hpp>

namespace devkit {

    class EnsureServiceIsHealthyAction : public Action {
    public:
        EnsureServiceIsHealthyAction(std::string serviceName, bool startIfNotRunning)
            : serviceName(std::move(serviceName)), 
            startIfNotRunning(startIfNotRunning) { }

        void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
            auto& serviceOpt = appCtx.GetService(serviceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + serviceName);
            }
            auto& service = **serviceOpt;
            auto status = service.Status(pipeline.RefreshActiveProcesses());
            if (startIfNotRunning) {
                if (status == ServiceStatus::DOWN) {
                    Info("-- {} is down. Start it", serviceName);
                    service.Start(pipeline.ActiveProcesses());
                } else if (status == ServiceStatus::UNKNOWN) {
                    Info("-- {} is in unknown state");
                }
            }
            if (!service.WaitForHealthy()) {
                throw std::runtime_error(serviceName + " not healthy");
            }
        }

        std::string Description() override {
            return "Check " + serviceName;
        }

        bool Counting() {
            return false;
        }

    private:
        std::string serviceName;
        bool startIfNotRunning;
    };
}