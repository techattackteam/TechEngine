#include <ios>
#include <imgui_internal.h>
#include "SceneHierarchyPanel.hpp"
#include "scene/Scene.hpp"
#include "events/panels/CloseInspectorEvent.hpp"
#include "events/panels/OpenInspectorEvent.hpp"
#include "events/OnSelectGameObjectEvent.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel() : Panel("SceneHierarchyPanel") {

    }

    void SceneHierarchyPanel::onUpdate() {
        Scene &scene = Scene::getInstance();
        ImGui::Begin("Scene Hierarchy");
        if (!scene.getGameObjects().empty()) {
            for (auto element: Scene::getInstance().getGameObjects()) {
                drawEntityNode(element);
            }
/*            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
                m_SelectionContext = {};*/

/*            // Right-click on blank space
            if (ImGui::BeginPopupContextWindow(0, 1, false)) {
                if (ImGui::MenuItem("Create Empty Entity"))
                    m_Context->CreateEntity("Empty Entity");

                ImGui::EndPopup();
            }*/

        }
        ImGui::End();
    }

    void SceneHierarchyPanel::drawEntityNode(GameObject *gameObject) {
        std::string name = gameObject->getName();

        ImGuiTreeNodeFlags flags = ((selectedGO == gameObject) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void *) (uint64_t) (uint32_t) gameObject, flags, name.c_str());
        if (ImGui::IsItemClicked()) {
            selectedGO = gameObject;
            TechEngineCore::EventDispatcher::getInstance().dispatch(new OnSelectGameObjectEvent(gameObject));
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;

            ImGui::EndPopup();
        }

        if (opened) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool opened = ImGui::TreeNodeEx((void *) 9817239, flags, name.c_str());
            if (opened)
                ImGui::TreePop();
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f) {
        ImGuiIO &io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }
}
