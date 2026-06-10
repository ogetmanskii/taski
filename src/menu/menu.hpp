#pragma once

#include "../action/action.hpp"
#include "../context/app_context.hpp"
#include <functional>
#include <string>
#include <vector>
#include <optional>

namespace devkit {

    class MenuItem {
    public:
        MenuItem(
            std::string title,
            std::string description,
            std::function<void(AppContext&, PipelineContext&)> pipelineAction
        ) : title(std::move(title)),
            description(std::move(description)),
            pipelineAction(std::move(pipelineAction)),
            length(GetTerminalLength(this->title))
        { }

        const std::string title;
        const std::string description;
        const std::function<void(AppContext&, PipelineContext&)> pipelineAction;
        const int length;
    };

    bool ShowMenu(const std::vector<MenuItem>& items, std::shared_ptr<AppContext> appContext);
}