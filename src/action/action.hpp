#pragma once

#include "../service/Service.hpp"
#include "../task/Task.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <algorithm>
#include <unordered_set>

namespace devkit {

    class AppContext;

    class PipelineContext;

    class Action  {
    public:
        virtual ~Action() = default;
        virtual void Run(AppContext& appCtx, PipelineContext& pipeline) = 0;
        virtual std::string Description() = 0;
        virtual bool Counting() = 0;
    };
}