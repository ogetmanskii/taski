#pragma once
#include "Action.hpp"

namespace devkit {
    class RunTaskAction : public Action {
    public:
        RunTaskAction(std::shared_ptr<Task> task)
            : task(std::move(task)) {
        }

        void Run(AppContext& appCtx, PipelineContext& pipeline) override {
            task->Run();
        }

        std::string Description() override {
            return task->GetName();
        }

        bool Counting() {
            return true;
        }

    private:
        std::shared_ptr<Task> task;
    };
}