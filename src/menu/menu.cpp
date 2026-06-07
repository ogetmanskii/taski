#include "menu.hpp"
#include "../logging/console_logging.hpp"
#include "../util/util.hpp"
#include "../context/app_context.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <format>
#include <vector>
#include <algorithm>
#include <chrono>

#include <string>
#include <utility>

#define NOMINMAX
#include <windows.h>

namespace devkit {

    static std::string GetShortcut(int i, const MenuItem& item) {
        if (item.shortcut) {
            return *item.shortcut;
        } else {
            return std::to_string(i);
        }
    }

    static int GetConsoleWidth() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            return csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
        return 80;
    }

    static int GetConsoleHeight() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
        return 25;
    }

    struct MenuChoice {
        std::string key;
        const std::string* rawTitle;
        const std::string* decoratedTitle;
    };

    static void PrintMenuColumns(
        std::vector<MenuChoice> menuChoices,
        const std::string& prompt = "> ",
        int minKeyWidth = 1,
        int columnSpacing = 1) {

        int consoleWidth = GetConsoleWidth();
        int consoleHeight = GetConsoleHeight();

        // Находим максимальную длину ключа
        size_t maxKeyWidth = minKeyWidth;
        for (const auto& item : menuChoices) {
            maxKeyWidth = std::max(maxKeyWidth, item.key.length());
        }

        // Вычисляем максимальную длину элемента с учетом цветовых кодов
        size_t maxVisualWidth = 0;
        for (const auto& item : menuChoices) {
            size_t visualWidth = maxKeyWidth + 1 + item.rawTitle->length();
            maxVisualWidth = std::max(maxVisualWidth, visualWidth);
        }

        maxVisualWidth += columnSpacing;

        int maxColumnsByWidth = std::max(1, consoleWidth / static_cast<int>(maxVisualWidth));
        int availableHeight = std::max(1, consoleHeight - 2);
        int optimalRowsPerColumn = std::min(availableHeight, static_cast<int>(menuChoices.size()));
        int numColumns = (menuChoices.size() + optimalRowsPerColumn - 1) / optimalRowsPerColumn;

        numColumns = std::min(numColumns, maxColumnsByWidth);

        // Если колонки не помещаются по ширине, пересчитываем количество строк
        int numRows;
        if (numColumns == maxColumnsByWidth && numColumns > 0) {
            // Распределяем элементы равномерно по доступным колонкам
            numRows = (menuChoices.size() + numColumns - 1) / numColumns;
        } else {
            numRows = optimalRowsPerColumn;
        }

        // Выводим меню
        for (int row = 0; row < numRows; ++row) {
            for (int col = 0; col < numColumns; ++col) {
                int index = row + col * numRows;

                if (index < menuChoices.size()) {
                    const auto& item = menuChoices[index];

                    std::string paddedKey = PadRight(item.key, maxKeyWidth);
                    std::cout << color::cyan(paddedKey) << " " << *item.decoratedTitle;

                    int currentWidth = maxKeyWidth + 1 + item.rawTitle->length();
                    int padding = maxVisualWidth - columnSpacing - currentWidth;

                    if (col < numColumns - 1 && index + numRows < menuChoices.size()) {
                        std::cout << std::string(padding + columnSpacing, ' ');
                    }
                }
            }
            std::cout << '\n';
        }

        std::cout << '\n' << prompt;
        std::cout.flush();
    }

    // Возвращает true, если что-то выбрано в меню
    bool ShowMenu(const std::vector<MenuItem>& items, std::shared_ptr<AppContext> appContext) {

        std::unordered_map<std::string, const MenuItem*> actions;
        size_t minWidth = 1;

        std::vector<MenuChoice> menuChoices;
        int number = 1;
        for (const MenuItem& item : items) {
            std::string shortcut = GetShortcut(number++, item);
            actions[shortcut] = &item;
            minWidth = std::max(shortcut.size(), minWidth);
            menuChoices.push_back({
                .key = std::move(shortcut),
                .rawTitle = &item.rawTitle,
                .decoratedTitle = &item.decoratedTitle
            });
        }
        PrintMenuColumns(menuChoices);

        std::string line;
        std::getline(std::cin, line);
        ClearScreen();

        bool correctPipeline = true;
        bool anySelected = false;
        PipelineContext pipeline(appContext);
        std::string action;
        std::stringstream sstream(line);
        while (std::getline(sstream, action, ' ')) {
            if (action.empty()) {
                continue;
            }
            anySelected = true;
            const MenuItem* item = actions[action];
            if (item) {
                try {
                    item->pipelineAction(*appContext, pipeline);
                } catch (const std::exception& e) {
                    info("-- Incorrect pipeline: {}", e.what());
                    correctPipeline = false;
                }
            } else {
                correctPipeline = false;
                std::cout << "-- Unknown choice: " << action << std::endl;
            }
        }
        if (correctPipeline) {
            try {
                pipeline.Run();
            } catch (const std::exception& e) {
                info("{}: {}", color::red("Error"), e.what());
                return true;
            }
        }

        return anySelected;
    }
}