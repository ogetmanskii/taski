#pragma once

#include <string>

namespace devkit {

    class ApplicationContext;

    class Pipeline;

    class Action  {
    public:
        virtual ~Action() = default;
        virtual void Run(ApplicationContext& appCtx, Pipeline& pipeline) = 0;
        virtual std::string Description() = 0;
        virtual bool Counting() = 0;
    };
}