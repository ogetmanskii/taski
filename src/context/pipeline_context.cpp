#include "app_context.hpp"
#include "../action/action.hpp"
#include <exception>
#include <chrono>

namespace devkit {
    void PipelineContext::Run() {
        int total = actions.size();
        if (total == 0) {
            return;
        }
        auto pipelineStart = std::chrono::high_resolution_clock::now();
        int i = 1;
        for (auto& action : actions) {
            info("\n-- [{}/{}] {}", i, total, action->Description());
            try {
                action->Run(*appContext, *this);
                i++;
            } catch (const std::exception& e) {
                std::string message = std::format("-- [{}/{}] {}: failed: {}", i, total, action->Description(), e.what());
                info(message);
                throw e;
            }
        }
        auto pipelineEnd = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
        info("\n-- {}: completed in {}", color::green("OK"), DurationFormatter::Format(totalMs));
    }
}