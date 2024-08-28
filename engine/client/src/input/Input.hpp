#pragma once

#include "Mouse.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CLIENT_DLL Input : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Mouse mouse;

    public:
        Input(SystemsRegistry& systemsRegistry);

        void init() override;

        void onKeyInput(int key, int action);
    };
}
