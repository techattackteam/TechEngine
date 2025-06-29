#include "UIHierarchyPanel.hpp"

#include <RmlUi/Core/ElementDocument.h>

#include "UIEditor.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
    }

    void UIHierarchyPanel::onUpdate() {
        for (int i = 0; i < m_context->GetNumDocuments(); ++i) {
            Rml::ElementDocument* doc = m_context->GetDocument(i);
            if (!doc) {
                continue;
            }
            for (int child_idx = 0; child_idx < doc->GetNumChildren(); ++child_idx) {
                m_editor->drawRmlElementInHierarchy(doc->GetChild(child_idx));
            }
        }
    }

    void UIHierarchyPanel::setEditor(UIEditor* editor) {
        m_editor = editor;
    }
}
