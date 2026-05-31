#pragma once

#include "../action/action.hpp"
#include "../context/app_context.hpp"
#include <functional>
#include <string>
#include <vector>
#include <optional>

namespace devkit {
    struct MenuItem {
        std::string title;
        std::string description;
        std::optional<std::string> shortcut;
        std::function<void(AppContext&, PipelineContext&)> pipelineAction;
    };

    bool ShowMenu(const std::vector<MenuItem>& items, std::shared_ptr<AppContext> appContext);
}