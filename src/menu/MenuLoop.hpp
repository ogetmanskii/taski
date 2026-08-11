#pragma once

#include <string>
#include <vector>
#include <memory>
#include <conio.h>

#include <Plan.hpp>
#include <menu/Menu.hpp>
#include <menu/MenuItem.hpp>

using namespace devkit::Menu;

namespace devkit {
    class ApplicationContext;
}

namespace {
    constexpr char ENTER = '\r';
    constexpr char ESCAPE = 27;

    inline bool IsReturn(const char c) {
        return c == ENTER || c == ESCAPE;
    }
}

namespace devkit::Menu {

    void RunLoop(std::shared_ptr<ApplicationContext> context) {
        while (true) {
            auto activeProcesses = GetActiveProcesses();
            std::vector<MenuItem> menu;
            if (context->GetServices().size() > 1) {
                menu.push_back(MenuItem("Start All", "Start All Services", Plan::PlanStartAllServices));
                menu.push_back(MenuItem("Stop All", "Stop All Services", Plan::PlanStopAllServices));
            }
            for (auto& service : context->GetServices()) {
                auto status = service->Status(activeProcesses);
                const std::string& serviceName = service->definition.name;
                if (status == ServiceStatus::UP) {
                    menu.push_back(MenuItem(
                        std::format("{} {}", serviceName, Color::Green("on")),
                        std::format("Stop {}", serviceName),
                        [&](ApplicationContext& ctx, Pipeline& pipeline) {
                        Plan::PlanStopService(ctx, pipeline, service);
                    }));
                } else if (status == ServiceStatus::DOWN) {
                    menu.push_back(MenuItem(
                        std::format("{} {}", serviceName, Color::Gray("off")),
                        std::format("Start {}", serviceName),
                        [&](ApplicationContext& ctx, Pipeline& pipeline) {
                        Plan::PlanStartService(ctx, pipeline, service);
                    }
                    ));
                } else {
                    menu.push_back(MenuItem(
                        serviceName,
                        std::format("Run {}", serviceName),
                        [&](ApplicationContext& ctx, Pipeline& pipeline) {
                        Plan::PlanStartService(ctx, pipeline, service);
                    }
                    ));
                }
            }
            for (auto& task : context->GetTasks()) {
                if (task->IsHidden()) {
                    continue;
                }
                menu.push_back(MenuItem(
                    task->GetName(),
                    task->GetName(),
                    [&](ApplicationContext& ctx, Pipeline& pipeline) {
                    Plan::PlanTask(ctx, pipeline, task);
                }
                ));
            }

            if (Menu::Show(menu, context)) {
                while (!IsReturn(_getch())) {

                }
            }
            ClearScreen();
        }
    }
}