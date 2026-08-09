#pragma once

#include <memory>
#include <exception>
#include <chrono>

#include "AppContext.hpp"
#include "../action/Action.hpp"
#include "../processes/Processes.hpp"

namespace devkit {

    using namespace Processes;

    class Action;

    class PipelineContext {

    public:
        PipelineContext(std::shared_ptr<AppContext> appContext)
            : appContext(std::move(appContext)) {
        }

        bool InsertKey(const std::string& key) {
            return keys.insert(key).second;
        }

        void AddAction(std::shared_ptr<Action> action) {
            actions.push_back(action);
        }

        void Run();

        std::vector<ProcessInfo>& ActiveProcesses() {
            if (!activeProcesses) {
                activeProcesses = GetActiveProcesses();
            }
            return *activeProcesses;
        }

        std::vector<ProcessInfo>& RefreshActiveProcesses() {
            activeProcesses = GetActiveProcesses();
            return *activeProcesses;
        }

    private:
        std::optional<std::vector<ProcessInfo>> activeProcesses;

        std::shared_ptr<AppContext> appContext;
        std::vector<std::shared_ptr<Action>> actions;
        std::unordered_set<std::string> keys;
    };
}