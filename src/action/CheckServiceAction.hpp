#pragma once

#include <exception>

#include <action/Action.hpp>
#include <Pipeline.hpp>

namespace devkit {

    class CheckServiceAction : public Action {
    public:
        CheckServiceAction(std::string serviceName, bool autoStart)
            : serviceName(std::move(serviceName)), 
            autoStart(autoStart) { }

        void Run(ApplicationContext& appCtx, Pipeline& pipeline) override {
            auto& serviceOpt = appCtx.GetService(serviceName);
            if (!serviceOpt) {
                throw std::runtime_error("No such service: " + serviceName);
            }
            auto& service = **serviceOpt;
            if (autoStart) {
                auto status = service.Status(pipeline.RefreshActiveProcesses());
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
        bool autoStart;
    };
}