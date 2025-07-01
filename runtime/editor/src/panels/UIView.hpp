#pragma once
#include "Panel.hpp"

#include <RmlUi/Core/Context.h>

#include "UIUtils/Guizmo.hpp"


namespace TechEngine {
    class UIEditor;

    class UIView : public Panel {
    private:
        UIEditor* m_uiEditor = nullptr;
        SystemsRegistry& m_appSystemsRegistry;

        uint32_t m_frameBufferID = 0;
        Rml::Context* m_context = nullptr;
        ImVec2 m_lastMousePosition;
        Guizmo guizmo;
        int id = 1000; // Unique ID for the view, can be used for guizmo operations

    public:
        UIView(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry, UIEditor* uiEditor);

        void onInit() override;

        void onUpdate() override;
    };
}
