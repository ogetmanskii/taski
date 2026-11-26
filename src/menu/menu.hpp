#pragma once

#include <memory>
#include <vector>

#include "menu/MenuItem.hpp"

namespace devkit {

class ApplicationContext;

}

namespace devkit::Menu {

// Показывает интерактивное меню. ioCursor — входное начальное и выходное
// итоговое положение курсора: позиция выбранного пункта сохраняется между
// отрисовками меню (позиционируется в пределах нового списка).
bool Show(const std::vector<MenuItem>& items, ApplicationContext& appContext, int& ioCursor);

}