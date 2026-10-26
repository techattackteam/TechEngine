#pragma once

namespace TechEngine {
    // Every call runs on the render thread with the GL context current.
    class ImGuiHook {
    public:
        virtual ~ImGuiHook() = default;

        virtual void start() = 0;

        virtual void draw() = 0;

        virtual void stop() = 0;
    };
}
