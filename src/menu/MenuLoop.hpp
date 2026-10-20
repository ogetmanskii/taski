#pragma once

#include <conio.h>

#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Pipeline.hpp"
#include "Plan.hpp"
#include "console/Color.hpp"
#include "console/Console.hpp"
#include "menu/Menu.hpp"
#include "menu/MenuItem.hpp"
#include "processes/Processes.hpp"
#include "service/Service.hpp"

namespace devkit {

class ApplicationContext;

}

namespace {

constexpr char ENTER = '\r';
constexpr char ESCAPE = 27;

inline bool IsReturn(const char c) { return c == ENTER || c == ESCAPE; }

}  // namespace

namespace devkit::Menu {

void RunLoop(std::shared_ptr<ApplicationContext> context) {
  ClearScreen();
  size_t serviceCount = context->GetServices().size();
  size_t taskCount = context->GetTasks().size();
  while (true) {
    auto activeProcesses = GetActiveProcesses();
    std::vector<MenuItem> menu;
    menu.reserve(serviceCount + taskCount + 2);
    int startAllItemIdx = -1;
    int stopAllItemIdx = -1;
    if (serviceCount > 1) {
      menu.push_back(MenuItem("Start All", "Start All Services", Plan::PlanStartAllServices));
      startAllItemIdx = menu.size() - 1;
      menu.push_back(MenuItem("Stop All", "Stop All Services", Plan::PlanStopAllServices));
      stopAllItemIdx = menu.size() - 1;
    }

    bool noActiveServices = true;
    bool noInactiveServices = true;
    for (auto& service : context->GetServices()) {
      auto status = service->Status(activeProcesses);
      const std::string& serviceName = service->definition.name;
      if (status == ServiceStatus::UP) {
        noActiveServices = false;
        menu.push_back(MenuItem(
            std::format("{} {}", serviceName, Color::Green("on")),
            std::format("Stop {}", serviceName),
            [&](ApplicationContext& ctx, Pipeline& pipeline) { Plan::PlanStopService(ctx, pipeline, service); }));
      } else if (status == ServiceStatus::DOWN) {
        noInactiveServices = false;
        menu.push_back(MenuItem(
            std::format("{} {}", serviceName, Color::Gray("off")),
            std::format("Start {}", serviceName),
            [&](ApplicationContext& ctx, Pipeline& pipeline) { Plan::PlanStartService(ctx, pipeline, service); }));
      } else {
        menu.push_back(
            MenuItem(serviceName, std::format("Run {}", serviceName), [&](ApplicationContext& ctx, Pipeline& pipeline) {
              Plan::PlanStartService(ctx, pipeline, service);
            }));
      }
    }
    if (noInactiveServices && startAllItemIdx != -1) {
      menu[startAllItemIdx].MakeGray();
    }
    if (noActiveServices && stopAllItemIdx != -1) {
      menu[stopAllItemIdx].MakeGray();
    }
    for (auto& task : context->GetTasks()) {
      if (task->IsHidden()) {
        continue;
      }
      menu.push_back(MenuItem(task->GetName(), task->GetName(), [&](ApplicationContext& ctx, Pipeline& pipeline) {
        Plan::PlanTask(ctx, pipeline, task);
      }));
    }

    if (Menu::Show(menu, context)) {
      while (!IsReturn(_getch())) {
      }
    }
    ClearScreen();
  }
}

}  // namespace devkit::Menu