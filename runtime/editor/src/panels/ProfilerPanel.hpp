#pragma once
#include "Panel.hpp"

namespace TechEngine {
    class ProfilerPanel : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;

    public:
        ProfilerPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;
    };
}
