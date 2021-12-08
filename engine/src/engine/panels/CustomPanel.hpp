#pragma once

#include <imgui.h>
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


