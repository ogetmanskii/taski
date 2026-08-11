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

#include <stdlib.h>
#include <conio.h>


#include <ApplicationContext.hpp>
#include <Pipeline.hpp>
#include <Plan.hpp>
#include <console/Console.hpp>
#include <console/Color.hpp>
#include <menu/Menu.hpp>
#include <menu/MenuItem.hpp>

using namespace devkit;
using namespace devkit::Menu;

namespace devkit::Menu {
    struct MenuChoice {
        const std::string key;
        const std::string* title;
        const int width;
    };
}

namespace {

    void PrintMenuColumns(
    std::vector<MenuChoice> menuChoices,
    const std::string& prompt = "> ",
    int minKeyWidth = 1,
    int columnSpacing = 2) {

        auto bounds = Console::GetConsoleBounds();
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
        int optimalRowsPerColumn = std::min(availableHeight, std::max(1, static_cast<int>(menuChoices.size())));
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

}

namespace devkit::Menu {

    using namespace devkit;
    using namespace devkit::Console;

    // Возвращает true, если что-то выбрано в меню
    bool Show(const std::vector<MenuItem>& items, std::shared_ptr<ApplicationContext> appContext) {

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
        Pipeline pipeline(appContext);
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
                pipeline.Execute();
            } catch (const std::exception& e) {
                Info("{}: {}", Color::Red("Error"), e.what());
                return true;
            }
        }

        return anySelected;
    }
}