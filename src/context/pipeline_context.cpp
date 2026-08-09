#include "app_context.hpp"
#include "../action/action.hpp"
#include <exception>
#include <chrono>

namespace devkit {
    void PipelineContext::Run() {
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
                info("\n-- [{}/{}] {}", i, total, action->Description());
                i++;
            } else {
                info("\n-- {}", action->Description());
            }
            try {
                action->Run(*appContext, *this);
            } catch (const std::exception& e) {
                std::string message = std::format("-- {}: failed: {}", action->Description(), e.what());
                info(message);
                throw e;
            }
        }
        auto pipelineEnd = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
        info("\n-- {}: completed in {}", color::green("OK"), DurationFormatter::Format(totalMs));
    }
}