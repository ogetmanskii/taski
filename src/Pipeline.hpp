#pragma once

#include <memory>
#include <exception>
#include <chrono>
#include <optional>

#include <processes/Processes.hpp>
#include <processes/ProcessDescriptor.hpp>
#include <ApplicationContext.hpp>
#include <console/Color.hpp>
#include <console/DurationFormatter.hpp>
#include <action/Action.hpp>

namespace devkit {

    using namespace Processes;
    using namespace Console;
    using namespace Console::Color;

    class Pipeline {

    public:
        Pipeline(std::shared_ptr<ApplicationContext> appContext)
            : appContext(std::move(appContext)) {
        }

        bool InsertKey(const std::string& key) {
            return keys.insert(key).second;
        }

        void PlanAction(std::shared_ptr<Action> action) {
            actions.push_back(action);
        }

        void PlanPostAction(const std::string& key, std::shared_ptr<Action> action) {
            postActions.push_back(std::make_pair(key, action));
        }

        std::vector<ProcessDescriptor>& ActiveProcesses() {
            if (!activeProcesses) {
                activeProcesses = GetActiveProcesses();
            }
            return *activeProcesses;
        }

        std::vector<ProcessDescriptor>& RefreshActiveProcesses() {
            activeProcesses = GetActiveProcesses();
            return *activeProcesses;
        }

        void Execute() {
            for (std::pair<std::string, std::shared_ptr<Action>> postAction : postActions) {
                if (InsertKey(postAction.first)) {
                    actions.push_back(postAction.second);
                }
            }
            if (actions.empty()) {
                return;
            }
            int total = 0;
            for (auto& action : actions) {
                if (action->Counting()) {
                    total++;
                }
            }
            auto pipelineStart = std::chrono::high_resolution_clock::now();
            int i = 1;
            for (auto& action : actions) {
                if (action->Counting()) {
                    Console::Info("-- [{}/{}] {}", i, total, action->Description());
                    i++;
                } else {
                    Console::Info("-- {}", action->Description());
                }
                try {
                    action->Run(*appContext, *this);
                } catch (const std::exception& e) {
                    std::string message = std::format("-- {} - failed: {}", action->Description(), e.what());
                    Console::Info(message);
                    throw e;
                }
            }
            auto pipelineEnd = std::chrono::high_resolution_clock::now();
            auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
            Console::Info("-- {}: completed in {}", Color::Green("OK"), DurationFormatter::Format(totalMs));
        }

    private:
        std::optional<std::vector<ProcessDescriptor>> activeProcesses;

        std::shared_ptr<ApplicationContext> appContext;
        std::vector<std::shared_ptr<Action>> actions;
        std::vector < std::pair<std::string /* key */, std::shared_ptr<Action>>> postActions;
        std::unordered_set<std::string> keys;
    };
}