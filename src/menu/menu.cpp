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

namespace devkit {

    static std::string GetShortcut(int i, const MenuItem& item) {
        if (item.shortcut) {
            return *item.shortcut;
        } else {
            return std::to_string(i);
        }
    }

    // Возвращает true, если что-то выбрано в меню
    bool ShowMenu(const std::vector<MenuItem>& items, std::shared_ptr<AppContext> appContext) {

        std::unordered_map<std::string, const MenuItem*> actions;
        size_t minWidth = 1;

        std::vector<std::pair<std::string /* shortcut */, const std::string* /* title */>> menuChoices;
        int number = 1;
        for (const MenuItem& item : items) {
            std::string shortcut = GetShortcut(number++, item);
            actions[shortcut] = &item;
            minWidth = std::max(shortcut.size(), minWidth);
            menuChoices.push_back(std::make_pair(std::move(shortcut), &item.title));
        }
        for (std::pair<std::string, const std::string*>& choice : menuChoices) {
            info(" {} {}", color::cyan(PadRight(std::move(choice.first), minWidth)), *choice.second);
        }
        std::cout << std::endl << PadRight("> ", minWidth + 1);
        std::cout.flush();

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