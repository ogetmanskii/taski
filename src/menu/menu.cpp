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

        int maxKeyWidth = minKeyWidth;
        for (const auto& item : menuChoices) {
            maxKeyWidth = std::max(maxKeyWidth, static_cast<int>(item.key.length()));
        }

        int availableHeight = std::max(1, consoleHeight - 2);

        auto getItemWidth = [&](const MenuChoice& item) -> int {
            return maxKeyWidth + 1 + item.width;
        };

        // Функция для проверки, помещается ли распределение в консоль
        auto tryDistribution = [&](
            int numColumns, 
            std::vector<int>& columnWidths,
            std::vector<int>& columnStartIndices
        ) -> bool {

            int numRows = (menuChoices.size() + numColumns - 1) / numColumns;
            columnWidths.assign(numColumns, 0);
            columnStartIndices.assign(numColumns + 1, 0);

            // Вычисляем границы столбцов
            for (int col = 0; col < numColumns; ++col) {
                columnStartIndices[col] = col * numRows;
                for (int row = 0; row < numRows; ++row) {
                    int index = row + col * numRows;
                    if (index < menuChoices.size()) {
                        columnWidths[col] = std::max(columnWidths[col],
                                                     getItemWidth(menuChoices[index]));
                    }
                }
            }
            columnStartIndices[numColumns] = menuChoices.size();

            // Проверяем общую ширину
            int totalWidth = 0;
            for (int col = 0; col < numColumns; ++col) {
                totalWidth += columnWidths[col];
                if (col < numColumns - 1) {
                    totalWidth += columnSpacing;
                }
            }

            return totalWidth <= consoleWidth;
        };

        // Находим оптимальное количество столбцов
        int optimalRowsPerColumn = std::min(
            availableHeight, 
            std::max(
                1, 
                static_cast<int>(menuChoices.size()))
        );
        int maxColumns = std::max(1ULL, (menuChoices.size() + optimalRowsPerColumn - 1) /
                                  optimalRowsPerColumn);

        std::vector<int> bestColumnWidths;
        std::vector<int> bestColumnStartIndices;
        int bestNumColumns = 1;

        // Пробуем разные количества столбцов, начиная с максимального
        for (int numColumns = maxColumns; numColumns >= 1; --numColumns) {
            std::vector<int> columnWidths;
            std::vector<int> columnStartIndices;

            if (tryDistribution(numColumns, columnWidths, columnStartIndices)) {
                bestNumColumns = numColumns;
                bestColumnWidths = columnWidths;
                bestColumnStartIndices = columnStartIndices;
                break;
            }
        }

        // Если не нашли подходящее распределение, используем одну колонку
        if (bestColumnWidths.empty()) {
            bestNumColumns = 1;
            tryDistribution(1, bestColumnWidths, bestColumnStartIndices);
        }

        int numRows = (menuChoices.size() + bestNumColumns - 1) / bestNumColumns;

        // Выводим меню
        for (int row = 0; row < numRows; ++row) {
            for (int col = 0; col < bestNumColumns; ++col) {
                int index = row + bestColumnStartIndices[col];

                if (index < bestColumnStartIndices[col + 1] && index < menuChoices.size()) {
                    const auto& item = menuChoices[index];

                    std::string paddedKey = PadRight(item.key, maxKeyWidth);
                    std::cout << Color::Cyan(paddedKey) << " " << *item.title;

                    int currentWidth = getItemWidth(item);
                    int padding = bestColumnWidths[col] - currentWidth;

                    if (col < bestNumColumns - 1 &&
                        row + bestColumnStartIndices[col + 1] < menuChoices.size()) {
                        std::cout << std::string(padding + columnSpacing, ' ');
                    }
                }
            }
            std::cout << '\n';
        }
        std::cout << prompt;
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