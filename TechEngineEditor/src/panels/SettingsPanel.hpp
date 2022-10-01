#pragma once

#include "Panel.hpp"

namespace TechEngine {
    class SettingsPanel : public Panel {
    public:
        ~SettingsPanel() override;

        void onUpdate() override;
    };
}
