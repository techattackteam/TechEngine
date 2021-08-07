#pragma once

#include <string>
#include "Panel.hpp"

namespace Engine {
    class ImGuiPanel : public Panel {
    private:

    public:

        explicit ImGuiPanel(const std::string &name);

        ~ImGuiPanel() override;

        void onUpdate() override;

    };
}


