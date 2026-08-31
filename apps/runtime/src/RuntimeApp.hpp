#pragma once

#include "TechEngine/app/App.hpp"

namespace TechEngine {
    class RuntimeApp : public App {
    public:
        RuntimeApp();

        ~RuntimeApp() override;

        static Role runtimeRole();

    protected:
        void init() override;

        void fixedUpdate(const FrameContext& frame) override;

        void update(const FrameContext& frame) override;

        void shutdown() override;
    };
}
