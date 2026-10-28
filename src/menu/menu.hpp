#pragma once

#include <memory>
#include <vector>

#include "menu/MenuItem.hpp"

namespace devkit {

class ApplicationContext;

}

namespace devkit::Menu {

bool Show(const std::vector<MenuItem>& items, ApplicationContext& appContext);

}