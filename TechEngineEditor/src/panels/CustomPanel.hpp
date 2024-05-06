#pragma once

#include <imgui.h>
#include <string>
#include "Panel.hpp"

namespace TechEngine {
    class CustomPanel : public Panel {
    protected:
        ImGuiContext* imGuiContext;

    public:
        CustomPanel(const std::string& name, EventDispatcher& eventDispatcher);

        ~CustomPanel() override;

        void setupImGuiContext(ImGuiContext* imGuiContext);
    };
}
