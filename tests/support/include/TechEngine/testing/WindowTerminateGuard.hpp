#pragma once

#include <TechEngine/platform/window/Window.hpp>

namespace TechEngineTests {
    struct WindowTerminateGuard {
        ~WindowTerminateGuard() {
            TechEngine::Window::terminate();
        }
    };
}
