#pragma once

#include <string>
#include <functional>
#include <memory>
#include <string_view>

namespace devkit {
    class ApplicationContext;
    class Pipeline;
}

namespace devkit::Console {
    int GetTerminalLength(const std::string_view str);
}

namespace devkit::Menu {

    class MenuItem {
    public:
        MenuItem(
            std::string title,
            std::string description,
            std::function<void(devkit::ApplicationContext&, devkit::Pipeline&)> pipelineAction
        ) : title(std::move(title)),
            description(std::move(description)),
            pipelineAction(std::move(pipelineAction)),
            length(Console::GetTerminalLength(this->title)) {
        }

        const std::string title;
        const std::string description;
        const std::function<void(devkit::ApplicationContext&, devkit::Pipeline&)> pipelineAction;
        const int length;
    };
}