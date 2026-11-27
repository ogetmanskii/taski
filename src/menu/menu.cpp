#include <algorithm>
#include <cctype>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define NOMINMAX
#include <Windows.h>

#include "ApplicationContext.hpp"
#include "Pipeline.hpp"
#include "console/Color.hpp"
#include "console/Console.hpp"
#include "menu/MenuItem.hpp"

using namespace devkit::Menu;

namespace devkit::Menu {

namespace {

// reservedRows — число строк под notice и строку справки, резервируемых вне
// списка, чтобы скользящее окно не выталкивало их за экран.
constexpr int RESERVED_ROWS = 3;

// menuEOL завершает строки меню. В raw-режиме вывод не переводит LF в CRLF,
// поэтому без \r каждая следующая строка начинается с той же колонки.
constexpr std::string_view MENU_EOL = "\r\n";

// Последовательности перерисовки кадра «на месте» (home + затирание строк и
// хвоста экрана) вместо ESC[2J: полная очистка экрана в VT-терминалах
// сбрасывает старый кадр в scrollback, плодя дубликаты меню в истории.
constexpr std::string_view HOME_SEQ = "\033[1;1H";
constexpr std::string_view ERASE_BELOW_SEQ = "\033[0J";
constexpr std::string_view ERASE_LINE_SEQ = "\033[K";
constexpr std::string_view HIDE_CURSOR_SEQ = "\033[?25l";
constexpr std::string_view SHOW_CURSOR_SEQ = "\033[?25h";

enum class MenuKey { None, Up, Down, Left, Right, Enter, Escape, Space, Refresh, Quit, Resize, End };

// RawModeGuard включает raw-режим консоли на время отрисовки и опроса меню и
// восстанавливает прежний режим при выходе из области видимости (в том числе
// при исключении). Во время выполнения выбранного пункта меню raw-режим
// выключается.
class RawModeGuard {
 public:
  RawModeGuard(HANDLE handle, DWORD savedMode) : handle(handle), savedMode(savedMode) {
    DWORD mode = (savedMode | ENABLE_WINDOW_INPUT) & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT | ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE);
    SetConsoleMode(handle, mode);
  }

  ~RawModeGuard() { Restore(); }

  RawModeGuard(const RawModeGuard&) = delete;
  RawModeGuard& operator=(const RawModeGuard&) = delete;

  void Restore() {
    if (active) {
      SetConsoleMode(handle, savedMode);
      active = false;
    }
  }

 private:
  HANDLE handle;
  DWORD savedMode;
  bool active = true;
};

// CursorRestorer прячет текстовый курсор терминала на время показа меню и
// возвращает его на всех выходах, иначе шелл остался бы с невидимым курсором.
class CursorRestorer {
 public:
  CursorRestorer() { std::cout << HIDE_CURSOR_SEQ << std::flush; }
  ~CursorRestorer() { std::cout << SHOW_CURSOR_SEQ << std::flush; }

  CursorRestorer(const CursorRestorer&) = delete;
  CursorRestorer& operator=(const CursorRestorer&) = delete;
};

inline void FrameHome() { std::cout << HOME_SEQ; }

inline void FrameEnd() { std::cout << ERASE_BELOW_SEQ << std::flush; }

// ClearFrame стирает видимую область без скролла: возврат в начало экрана и
// стирание вниз. Не создаёт записей в scrollback, в отличие от ESC[2J.
inline void ClearFrame() {
  FrameHome();
  FrameEnd();
}

// ReadKey читает одно нажатие. Стрелки приходят как KEY_EVENT с виртуальными
// кодами, изменение размера окна — как WINDOW_BUFFER_SIZE_EVENT (для него
// нужен ENABLE_WINDOW_INPUT в raw-режиме).
MenuKey ReadKey() {
  HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
  INPUT_RECORD record;
  DWORD read = 0;
  while (ReadConsoleInputW(input, &record, 1, &read) && read == 1) {
    if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
      return MenuKey::Resize;
    }
    if (record.EventType != KEY_EVENT) {
      continue;
    }
    const KEY_EVENT_RECORD& key = record.Event.KeyEvent;
    if (!key.bKeyDown) {
      continue;
    }
    switch (key.wVirtualKeyCode) {
      case VK_UP:
        return MenuKey::Up;
      case VK_DOWN:
        return MenuKey::Down;
      case VK_LEFT:
        return MenuKey::Left;
      case VK_RIGHT:
        return MenuKey::Right;
      case VK_RETURN:
        return MenuKey::Enter;
      case VK_ESCAPE:
        return MenuKey::Escape;
      case VK_SPACE:
        return MenuKey::Space;
      case 0x51:  // 'Q'
        if (key.uChar.UnicodeChar == L'q' || key.uChar.UnicodeChar == L'Q') {
          return MenuKey::Quit;
        }
        break;
      case 0x52:  // 'R'
        if (key.uChar.UnicodeChar == L'r' || key.uChar.UnicodeChar == L'R') {
          return MenuKey::Refresh;
        }
        break;
      default:
        break;
    }
  }
  return MenuKey::End;
}

// VisibleRows возвращает количество строк списка, помещающихся в терминал.
int VisibleRows(int total) {
  auto [width, height] = Console::GetConsoleBounds();
  (void)width;
  int rows = height - RESERVED_ROWS;
  if (rows < 1) {
    rows = 1;
  }
  if (rows > total) {
    rows = total;
  }
  return rows;
}

// TruncateToWidth обрезает строку так, чтобы её видимая длина (без учета
// ANSI escape-последовательностей, вычисляется Console::GetTerminalLength)
// уместилась в maxWidth колонок. Обрезанная строка помечается многоточием и
// сбросом цвета: срез может приходиться на середину окрашенного сегмента.
void TruncateToWidth(std::string& line, int maxWidth) {
  if (maxWidth <= 0) {
    line.clear();
    return;
  }
  if (Console::GetTerminalLength(line) <= maxWidth) {
    return;
  }
  size_t cut = line.size();
  size_t i = 0;
  int visible = 0;
  while (i < line.size()) {
    if (line[i] == '\033' && i + 1 < line.size() && line[i + 1] == '[') {
      // Пропускаем escape-последовательность, как в GetTerminalLength.
      i += 2;
      while (i < line.size() && !std::isalpha(static_cast<unsigned char>(line[i]))) {
        ++i;
      }
      if (i < line.size()) {
        ++i;
      }
      continue;
    }
    if (visible >= maxWidth - 1) {
      cut = i;
      break;
    }
    size_t charLen = 1;
    unsigned char c = static_cast<unsigned char>(line[i]);
    if (c >= 0xF0 && c < 0xF8) {
      charLen = 4;
    } else if (c >= 0xE0 && c < 0xF0) {
      charLen = 3;
    } else if (c >= 0xC0 && c < 0xE0) {
      charLen = 2;
    }
    if (i + charLen > line.size()) {
      charLen = 1;
    }
    i += charLen;
    ++visible;
  }
  line.resize(cut);
  line += "…";
  line += Color::RESET;
}

// MenuState хранит состояние интерактивного меню: позицию курсора,
// скользящее окно и отмеченные пункты в порядке выбора.
struct MenuState {
  const std::vector<MenuItem>& items;
  int cursor = 0;
  int offset = 0;  // первый видимый элемент списка (скользящее окно)
  std::unordered_set<int> selected;
  std::vector<int> selectedOrder;
  std::string notice;

  explicit MenuState(const std::vector<MenuItem>& items, int cursor = 0) : items(items), cursor(cursor) {}

  // SelectionNumbers возвращает номер запуска (с 1) для каждого выбранного пункта.
  std::unordered_map<int, int> SelectionNumbers() const {
    std::unordered_map<int, int> numbers;
    for (size_t i = 0; i < selectedOrder.size(); ++i) {
      numbers[selectedOrder[i]] = static_cast<int>(i) + 1;
    }
    return numbers;
  }

  void SelectAtCursor() {
    if (!selected.insert(cursor).second) {
      return;
    }
    selectedOrder.push_back(cursor);
  }

  void UnselectAtCursor() {
    if (selected.erase(cursor) == 0) {
      return;
    }
    auto it = std::find(selectedOrder.begin(), selectedOrder.end(), cursor);
    if (it != selectedOrder.end()) {
      selectedOrder.erase(it);
    }
  }

  void ToggleAtCursor() {
    if (selected.count(cursor)) {
      UnselectAtCursor();
    } else {
      SelectAtCursor();
    }
  }

  void MoveCursor(int d) {
    int total = static_cast<int>(items.size());
    if (total == 0) {
      return;
    }
    cursor += d;
    if (cursor < 0) {
      cursor = total - 1;
    }
    if (cursor > total - 1) {
      cursor = 0;
    }
  }

  // ScrollWindow сдвигает окно так, чтобы курсор всегда был видимым.
  void ScrollWindow(int total, int rows) {
    if (rows <= 0) {
      offset = 0;
      return;
    }
    if (cursor < offset) {
      offset = cursor;
    }
    if (cursor >= offset + rows) {
      offset = cursor - rows + 1;
    }
    if (offset < 0) {
      offset = 0;
    }
    int maxOffset = total - rows;
    if (offset > maxOffset) {
      offset = maxOffset;
    }
    if (offset < 0) {
      offset = 0;
    }
  }

  // BuildSelection определяет, какие пункты запускать: отмеченные или пункт
  // под курсором.
  std::vector<int> BuildSelection() const {
    if (!selectedOrder.empty()) {
      return selectedOrder;
    }
    if (cursor >= 0 && cursor < static_cast<int>(items.size())) {
      return {cursor};
    }
    return {};
  }

  // ClampState отбрасывает отмеченные пункты, вышедшие за границы списка
  // после перестроения меню; курсор клампится в Render.
  void ClampState() {
    int total = static_cast<int>(items.size());
    std::vector<int> kept;
    for (int idx : selectedOrder) {
      if (idx >= 0 && idx < total) {
        kept.push_back(idx);
      }
    }
    selectedOrder = std::move(kept);
    selected.clear();
    selected.insert(selectedOrder.begin(), selectedOrder.end());
  }

  void Render() {
    FrameHome();

    // Минус колонка запаса: многие терминалы переводят строку, если символ
    // попадает в последнюю колонку ширины.
    int maxLineWidth = std::max(1, Console::GetConsoleBounds().first - 1);

    int total = static_cast<int>(items.size());
    if (cursor < 0) {
      cursor = 0;
    }
    if (cursor > total - 1) {
      cursor = total - 1;
    }

    int rows = VisibleRows(total);
    ScrollWindow(total, rows);

    int end = offset + rows;
    if (end > total) {
      end = total;
    }

    auto numbers = SelectionNumbers();
    int numWidth = static_cast<int>(std::to_string(numbers.size()).size());
    for (int pos = offset; pos < end; ++pos) {
      const MenuItem& item = items[pos];

      bool isCursor = pos == cursor;
      auto it = numbers.find(pos);
      bool isSelected = it != numbers.end();

      std::string cursorMark = isCursor ? Color::Cyan("▶") : " ";

      std::string box;
      if (isSelected) {
        std::string number = std::to_string(it->second);
        box = Color::Cyan(std::string(static_cast<size_t>(numWidth) - number.size() + 1, ' ') + number);
      }

      std::string name = isSelected ? Color::Cyan(item.title) : item.title;

      std::string line = cursorMark + box + " " + name + " " + Color::Gray(item.description);
      TruncateToWidth(line, maxLineWidth);
      std::cout << line << ERASE_LINE_SEQ << MENU_EOL;
    }

    if (total == 0) {
      std::cout << "  no items" << ERASE_LINE_SEQ << MENU_EOL;
    }

    if (!notice.empty()) {
      std::string noticeLine = Color::Gray(notice);
      TruncateToWidth(noticeLine, maxLineWidth);
      std::cout << noticeLine << ERASE_LINE_SEQ << MENU_EOL;
    }
    std::string help = Color::Gray("↑/↓ move · space/←/→ select · enter run · r refresh · q quit");
    TruncateToWidth(help, maxLineWidth);
    std::cout << help << ERASE_LINE_SEQ << MENU_EOL;
    FrameEnd();
  }
};

}  // namespace

// Возвращает true, если что-то выбрано в меню.
// Навигация: стрелки вверх/вниз — курсор, вправо — отметить, влево — снять
// отметку, enter — запустить отмеченные в порядке выбора (или текущий пункт,
// если не отмечено ничего), r — перестроить меню, esc/q — выход.
// ioCursor — позиция курсора между отрисовками: стартовое значение задаёт
// начальную позицию, при выходе записывается итоговая (Render выполняет
// клампинг в пределы нового списка).
bool Show(const std::function<void(std::vector<MenuItem>&)>& buildMenu,
          ApplicationContext& appContext,
          int& ioCursor) {
  HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
  DWORD savedMode = 0;
  if (input == INVALID_HANDLE_VALUE || !GetConsoleMode(input, &savedMode)) {
    throw std::runtime_error("Menu requires an interactive terminal");
  }

  std::vector<MenuItem> items;
  buildMenu(items);

  MenuState state(items, ioCursor);
  bool anySelected = false;

  {
    RawModeGuard rawGuard(input, savedMode);
    CursorRestorer cursorRestorer;

    Console::ClearScreen();  // один раз при входе: содержимое шелла уезжает в scrollback
    state.Render();

    while (true) {
      switch (MenuKey key = ReadKey()) {
        case MenuKey::Escape:
        case MenuKey::Quit:
        case MenuKey::End:
          ioCursor = state.cursor;
          return anySelected;

        case MenuKey::Up:
          state.MoveCursor(-1);
          break;

        case MenuKey::Down:
          state.MoveCursor(1);
          break;

        case MenuKey::Right:
          state.SelectAtCursor();
          break;

        case MenuKey::Left:
          state.UnselectAtCursor();
          break;

        case MenuKey::Space:
          state.ToggleAtCursor();
          break;

        case MenuKey::Resize:
          break;

        case MenuKey::Refresh: {
          buildMenu(items);
          state.ClampState();
          state.notice = "menu refreshed";
          break;
        }

        case MenuKey::Enter: {
          std::vector<int> indices = state.BuildSelection();
          if (indices.empty()) {
            state.notice = "nothing to run";
            state.Render();
            continue;
          }

          ClearFrame();
          rawGuard.Restore();
          std::cout << SHOW_CURSOR_SEQ << std::flush;
          anySelected = true;
          ioCursor = state.cursor;

          Pipeline pipeline(appContext);
          bool correctPipeline = true;
          for (int idx : indices) {
            const MenuItem& item = items[idx];
            try {
              item.pipelineAction(appContext, pipeline);
            } catch (const std::exception& e) {
              Info("-- Incorrect pipeline: {}", e.what());
              correctPipeline = false;
            }
          }
          if (correctPipeline) {
            try {
              pipeline.Execute();
            } catch (const std::exception& e) {
              Info("{}: {}", Color::Red("Error"), e.what());
              return true;
            }
          }
          return true;
        }

        case MenuKey::None:
        default:
          continue;
      }
      state.Render();
    }
  }
}

}  // namespace devkit::Menu
