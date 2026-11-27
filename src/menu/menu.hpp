#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "menu/MenuItem.hpp"

namespace devkit {

class ApplicationContext;

}

namespace devkit::Menu {

// Показывает интерактивное меню, построенное buildMenu. ioCursor — входное
// начальное и выходное итоговое положение курсора: позиция выбранного пункта
// сохраняется между отрисовками меню (позиционируется в пределах нового
// списка). По кнопке r меню перестраивается вызовом buildMenu с актуальными
// состояниями сервисов on/off.
bool Show(const std::function<void(std::vector<MenuItem>&)>& buildMenu,
          ApplicationContext& appContext,
          int& ioCursor);

}