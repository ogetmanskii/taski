#include "Menu.hpp"
#include "MenuItem.hpp"
#include "../console/Console.hpp"
#include "../console/Color.hpp"
#include "../context/AppContext.hpp"
#include "../context/PipelineContext.hpp"

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

namespace devkit::Menu {

    using namespace devkit;
    using namespace devkit::Console;

    static std::pair<int /* width */, int /* height */> GetConsoleBounds() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            return std::make_pair(
                csbi.srWindow.Right - csbi.srWindow.Left + 1, 
                csbi.srWindow.Bottom - csbi.srWindow.Top + 1
            );
        }
        return std::make_pair(80, 25);
    }

    struct MenuChoice {
        const std::string key;
        const std::string* title;
        const int width;
    };

    static void PrintMenuColumns(
        std::vector<MenuChoice> menuChoices,
        const std::string& prompt = "> ",
        int minKeyWidth = 1,
        int columnSpacing = 2) {

        auto bounds = GetConsoleBounds();
        int& consoleWidth = bounds.first;
        int& consoleHeight = bounds.second;

        // Находим максимальную длину ключа и максимальную длину элемента
        int maxKeyWidth = minKeyWidth;
        int maxVisualWidth = 0;
        for (const auto& item : menuChoices) {
            maxKeyWidth = std::max(maxKeyWidth, static_cast<int>(item.key.length()));
            int visualWidth = maxKeyWidth + 1 + item.width;
            maxVisualWidth = std::max(maxVisualWidth, visualWidth);
        }

        maxVisualWidth += columnSpacing;

        int maxColumnsByWidth = std::max(1, consoleWidth / maxVisualWidth);
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
                    std::cout << Color::Cyan(paddedKey) << " " << *item.title;

                    int currentWidth = maxKeyWidth + 1 + item.width;
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
    bool Show(const std::vector<MenuItem>& items, std::shared_ptr<devkit::AppContext> appContext) {

        std::unordered_map<std::string, const MenuItem*> actions;
        size_t minWidth = 1;

        std::vector<MenuChoice> menuChoices;
        int number = 1;
        for (const MenuItem& item : items) {
            std::string shortcut = std::to_string(number++);
            actions[shortcut] = &item;
            minWidth = std::max(shortcut.size(), minWidth);
            menuChoices.push_back({
                .key = std::move(shortcut),
                .title = &item.title,
                .width = item.length
            });
        }
        PrintMenuColumns(menuChoices);

        std::string line;
        std::getline(std::cin, line);
        ClearScreen();

        bool correctPipeline = true;
        bool anySelected = false;
        devkit::PipelineContext pipeline(appContext);
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
                    Info("-- Incorrect pipeline: {}", e.what());
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
                Info("{}: {}", Color::Red("Error"), e.what());
                return true;
            }
        }

        return anySelected;
    }
}