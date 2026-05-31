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
        bool manySteps = total > 1;
        auto pipelineStart = std::chrono::high_resolution_clock::now();
        int i = 1;
        for (auto& action : actions) {
            if (manySteps) {
                info("\n-- [{}/{}] {}", i, total, action->Description());
            }
            try {
                action->Run(*appContext, *this);
                i++;
            } catch (const std::exception& e) {
                info("-- [{}/{}] {}: failed: {}", i, total, action->Description(), e.what());
                throw std::runtime_error("Stopped due to error");
            }
        }
        auto pipelineEnd = std::chrono::high_resolution_clock::now();
        auto pipelineMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
        info("\n-- {}: completed in {} ms", color::green("OK"), pipelineMs);
    }
}