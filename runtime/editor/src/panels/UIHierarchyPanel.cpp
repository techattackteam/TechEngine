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
        if (!ImGui::IsItemHovered() && ImGui::BeginPopupContextWindow("Widget Creation", 1)) {
            //if (ImGui::BeginPopupContextItem()) {
            if (ImGui::BeginMenu("Base Widgets")) {
                for (const Widget& widget: m_editor->getWidgetsRegistry().getBaseWidgets()) {
                    if (ImGui::MenuItem(widget.getName().c_str())) {
                        ImGui::Text("%s", widget.getName().c_str());
                        m_editor->createWidget(nullptr, widget.getName(), true);
                    }
                }
            ImGui::EndMenu();
            }
            //ImGui::EndPopup();
            //}
            for (const Widget& widget: m_editor->getWidgetsRegistry().getWidgets()) {
                if (ImGui::MenuItem(widget.getName().c_str())) {
                    ImGui::Text("%s", widget.getName().c_str());
                    m_editor->createWidget(nullptr, widget.getName(), false);
                }
            }
            ImGui::EndPopup();
        }
    }

    void UIHierarchyPanel::setEditor(UIEditor* editor) {
        m_editor = editor;
    }
}
