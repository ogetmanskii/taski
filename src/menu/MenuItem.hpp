#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <utility>

#include "console/Color.hpp"

namespace devkit {

class ApplicationContext;
class Pipeline;

}  // namespace devkit

namespace devkit::Console {

int GetTerminalLength(const std::string_view str);

}

namespace devkit::Menu {

class MenuItem {
 public:
  MenuItem(std::string title,
           std::string description,
           std::function<void(devkit::ApplicationContext&, devkit::Pipeline&)> pipelineAction)
      : title(std::move(title)),
        description(std::move(description)),
        pipelineAction(std::move(pipelineAction)),
        length(Console::GetTerminalLength(this->title)) {}

  void MakeGray() {
    std::string& mutableTitle = const_cast<std::string&>(title);
    mutableTitle = Color::Gray(mutableTitle);
  }

  const std::string title;
  const std::string description;
  const std::function<void(devkit::ApplicationContext&, devkit::Pipeline&)> pipelineAction;
  const int length;
};

}  // namespace devkit::Menu