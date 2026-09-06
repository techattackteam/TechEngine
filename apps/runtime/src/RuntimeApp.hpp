#pragma once

#include "TechEngine/app/App.hpp"

#include <cstdint>

namespace TechEngine {
    class RuntimeApp : public App {
    private:
        std::uint64_t m_frameCount = 0;

    public:
        RuntimeApp();

        ~RuntimeApp() override;

        static Role runtimeRole();

    protected:
        void init() override;

        void fixedUpdate(const FrameContext& frame) override;

        void update(const FrameContext& frame) override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
