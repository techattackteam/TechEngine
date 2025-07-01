#pragma once
#include <RmlUi/Core/Context.h>

#include "Panel.hpp"
#include "ui/Widget.hpp"

namespace TechEngine {
    class UIEditor;

    class UIHierarchyPanel : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        Rml::Context* m_context = nullptr;
        UIEditor* m_editor = nullptr;

        Widget* m_rootWidget = nullptr;
        Widget* m_selectedWidget = nullptr;

    public:
        UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry);

        void onInit() override;

        void onUpdate() override;

        void setEditor(UIEditor* editor);
    };
}
