#pragma once

#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include <string>
#include "Panel.hpp"

namespace Engine {
    class CustomPanel : public Panel {
    private:

    public:

        explicit CustomPanel(const std::string &name);

        ~CustomPanel() override;

        virtual void onUpdate() = 0;
    };
}


