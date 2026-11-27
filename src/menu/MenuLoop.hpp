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

void RunLoop(ApplicationContext& context) {
  ClearScreen();
  auto buildMenu = [&](std::vector<MenuItem>& menu) {
    size_t serviceCount = context.GetServices().size();
    size_t taskCount = context.GetTasks().size();
    auto activeProcesses = GetActiveProcesses();
    menu.clear();
    menu.reserve(serviceCount + taskCount + 2);
    int startAllItemIdx = -1;
    int stopAllItemIdx = -1;
    if (serviceCount > 1) {
      menu.push_back(MenuItem("Start All", "", Plan::PlanStartAllServices));
      startAllItemIdx = menu.size() - 1;
      menu.push_back(MenuItem("Stop All", "", Plan::PlanStopAllServices));
      stopAllItemIdx = menu.size() - 1;
    }

    bool noActiveServices = true;
    bool noInactiveServices = true;
    for (auto& servicePtr : context.GetServices()) {
      auto status = servicePtr->Status(activeProcesses);
      const std::string& serviceName = servicePtr->definition.name;
      if (status == ServiceStatus::UP) {
        noActiveServices = false;
        menu.push_back(MenuItem(
            std::format("{} {}{}{}", serviceName, Color::White("["), Color::Green("on"), Color::White("]")),
            "",
            [&](ApplicationContext& ctx, Pipeline& pipeline) { Plan::PlanStopService(ctx, pipeline, *servicePtr); }));
      } else if (status == ServiceStatus::DOWN) {
        noInactiveServices = false;
        menu.push_back(MenuItem(
            std::format("{} {}{}{}", serviceName, Color::White("["), Color::Gray("off"), Color::White("]")),
            "",
            [&](ApplicationContext& ctx, Pipeline& pipeline) { Plan::PlanStartService(ctx, pipeline, *servicePtr); }));
      } else {
        menu.push_back(
            MenuItem(serviceName, "", [&](ApplicationContext& ctx, Pipeline& pipeline) {
              Plan::PlanStartService(ctx, pipeline, *servicePtr);
            }));
      }
    }
    if (noInactiveServices && startAllItemIdx != -1) {
      menu[startAllItemIdx].MakeGray();
    }
    if (noActiveServices && stopAllItemIdx != -1) {
      menu[stopAllItemIdx].MakeGray();
    }
    for (auto& taskPtr : context.GetTasks()) {
      if (taskPtr->hidden) {
        continue;
      }
      menu.push_back(MenuItem(taskPtr->name, taskPtr->description.value_or(""), [&](ApplicationContext& ctx, Pipeline& pipeline) {
        Plan::PlanTask(ctx, pipeline, *taskPtr);
      }));
    }
  };

  int menuCursor = 0;
  while (true) {
    if (!Menu::Show(buildMenu, context, menuCursor)) {
      break;
    }
    while (!IsReturn(_getch())) {
    }
    ClearScreen();
  }
}

}  // namespace devkit::Menu