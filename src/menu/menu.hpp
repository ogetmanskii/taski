#pragma once

#include <string>
#include <vector>
#include <memory>

#include <menu/MenuItem.hpp>

namespace devkit {
    class ApplicationContext;
}

namespace devkit::Menu {
    bool Show(const std::vector<MenuItem>& items, std::shared_ptr<ApplicationContext> appContext);
}