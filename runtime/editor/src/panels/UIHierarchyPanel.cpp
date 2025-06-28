#include "UIHierarchyPanel.hpp"

#include <RmlUi/Core/ElementDocument.h>

#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    UIHierarchyPanel::UIHierarchyPanel(SystemsRegistry& editorSystemsRegistry, SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry), m_appSystemsRegistry(appSystemsRegistry) {
    }

    void UIHierarchyPanel::onInit() {
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
    }

    void UIHierarchyPanel::onUpdate() {
        if (m_context) {
            if (m_context->GetNumDocuments() > 0) {
                for (int i = 0; i < m_context->GetNumDocuments(); ++i) {
                    Rml::ElementDocument* doc = m_context->GetDocument(i);
                    if (doc) {
                        ImGuiTreeNodeFlags doc_node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                        bool doc_node_open = ImGui::TreeNodeEx((void*)doc, doc_node_flags, "Document: %s", doc->GetSourceURL().c_str());
                        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                            /* Select doc? */
                        }
                        if (doc_node_open) {
                            /*if (doc->GetBody()) drawElementInHierarchy(doc->GetBody());
                            else {
                            for (int child_idx = 0; child_idx < doc->GetNumChildren(); ++child_idx) {
                            drawElementInHierarchy(doc->GetChild(child_idx));
                            }
                            } */
                            ImGui::TreePop();
                        }
                    }
                }
            } else {
                ImGui::Text("No documents in context.");
            }
        } else {
            ImGui::Text("UI Context not initialized.");
        }
    }
}
