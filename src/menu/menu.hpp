#pragma once

#include "MenuItem.hpp"
#include <string>
#include <vector>

namespace devkit {
    class AppContext;
}

namespace devkit::Menu {

    bool Show(const std::vector<MenuItem>& items, std::shared_ptr<AppContext> appContext);
}