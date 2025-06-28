#pragma once
#include <RmlUi/Core/Context.h>

#include "Panel.hpp"

namespace TechEngine {
    class UIHierarchyPanel : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context = nullptr;

    public:
        UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;
    };
}
