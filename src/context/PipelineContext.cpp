#include "PipelineContext.hpp"
#include "../console/Color.hpp"
#include "../console/DurationFormatter.hpp"

namespace devkit {

    using namespace Console;
    using namespace Console::Color;

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
                Info("\n-- [{}/{}] {}", i, total, action->Description());
                i++;
            } else {
                Info("\n-- {}", action->Description());
            }
            try {
                action->Run(*appContext, *this);
            } catch (const std::exception& e) {
                std::string message = std::format("-- {}: failed: {}", action->Description(), e.what());
                Info(message);
                throw e;
            }
        }
        auto pipelineEnd = std::chrono::high_resolution_clock::now();
        auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
        Info("\n-- {}: completed in {}", Color::Green("OK"), DurationFormatter::Format(totalMs));
    }
}