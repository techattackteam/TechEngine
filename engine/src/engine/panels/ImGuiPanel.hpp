#pragma once

#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include <string>
#include "Panel.hpp"

namespace Engine {
    class ImGuiPanel : public Panel {
    private:

    public:

        explicit ImGuiPanel(const std::string &name);

        ~ImGuiPanel() override;

        virtual void onUpdate() = 0;
    };
}


