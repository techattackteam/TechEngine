#pragma once
#include "Panel.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    class GameView : public Panel {
    private:
        SystemsRegistry& m_appRegistry;
        uint32_t frameBufferID;
    public:
        GameView(SystemsRegistry& appRegistry);

        void onInit() override;

        void onUpdate() override;
    };
}
