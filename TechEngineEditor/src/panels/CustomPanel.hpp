#pragma once

#include <imgui.h>
#include <string>
#include "Panel.hpp"

namespace TechEngine {
    class CustomPanel : public Panel {
    protected:
        ImGuiContext* imGuiContext;

    public:
        CustomPanel(const std::string& name, SystemsRegistry& editorRegistry);

        ~CustomPanel() override;

        void init() override;

        void setupImGuiContext(ImGuiContext* imGuiContext);
    };
}
