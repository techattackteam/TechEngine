#include <imgui_internal.h>
#include "InspectorPanel.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel() : Panel("Inspector") {
        TechEngineCore::EventDispatcher::getInstance().subscribe(OnSelectGameObjectEvent::eventType, [this](TechEngineCore::Event *event) {
            inspectGameObject((OnSelectGameObjectEvent *) (event));
        });
    }


    void InspectorPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        if (gameObject != nullptr) {
            drawComponents();
        }
        ImGui::End();
    }

    template<typename T, typename UIFunction>
    void InspectorPanel::drawComponent(const std::string &name, UIFunction uiFunction) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (gameObject->hasComponent<T>()) {
            auto component = gameObject->getComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void *) typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
            ImGui::PopStyleVar();
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{lineHeight, lineHeight})) {
                ImGui::OpenPopup("ComponentSettings");
            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSettings")) {
                if (ImGui::MenuItem("Remove component"))
                    removeComponent = true;

                ImGui::EndPopup();
            }

            if (open) {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
                gameObject->removeComponent<T>();
        }
    }

    void InspectorPanel::drawComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");


        ImGui::PopItemWidth();

        drawComponent<TransformComponent>("Transform", [this](auto &component) {
            glm::vec3 position = component->position;
            glm::vec3 orientation = component->orientation;
            glm::vec3 scale = component->scale;
            drawVec3Control("Translation", position);
            drawVec3Control("Rotation", orientation);
            drawVec3Control("Scale", scale, 1.0f);
            if (position != component->position)
                component->translateTo(position);
            if (orientation != component->orientation)
                component->rotate(orientation);
            if (scale != component->scale)
                component->setScale(scale);
        });

        drawComponent<CameraComponent>("Camera", [](auto &component) {
                                           auto &camera = component;

                                           //ImGui::Checkbox("Primary", &component->Primary);

                                           const char *projectionTypeStrings[] = {"Perspective", "Orthographic"};
                                           const char *currentProjectionTypeString = projectionTypeStrings[(int) camera->getProjectionType()];
                                           if (ImGui::BeginCombo("Projection", currentProjectionTypeString)) {
                                               for (int i = 0; i < 2; i++) {
                                                   bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                                                   if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
                                                       currentProjectionTypeString = projectionTypeStrings[i];
                                                       camera->changeProjectionType((CameraComponent::ProjectionType) i);
                                                   }

                                                   if (isSelected)
                                                       ImGui::SetItemDefaultFocus();
                                               }

                                               ImGui::EndCombo();
                                           }

/*                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective) {
                                               float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
                                               if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                                                   camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

                                               float perspectiveNear = camera.GetPerspectiveNearClip();
                                               if (ImGui::DragFloat("Near", &perspectiveNear))
                                                   camera.SetPerspectiveNearClip(perspectiveNear);

                                               float perspectiveFar = camera.GetPerspectiveFarClip();
                                               if (ImGui::DragFloat("Far", &perspectiveFar))
                                                   camera.SetPerspectiveFarClip(perspectiveFar);
                                           }*/

                                           /*                                     if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic) {
                                                                                    float orthoSize = camera.GetOrthographicSize();
                                                                                    if (ImGui::DragFloat("Size", &orthoSize))
                                                                                        camera.SetOrthographicSize(orthoSize);

                                                                                    float orthoNear = camera.GetOrthographicNearClip();
                                                                                    if (ImGui::DragFloat("Near", &orthoNear))
                                                                                        camera.SetOrthographicNearClip(orthoNear);

                                                                                    float orthoFar = camera.GetOrthographicFarClip();
                                                                                    if (ImGui::DragFloat("Far", &orthoFar))
                                                                                        camera.SetOrthographicFarClip(orthoFar);

                                                                                    ImGui::Checkbox("Fixed Aspect Ratio", &component->FixedAspectRatio);
                                                                                }*/
                                       }

        );
    }

    void InspectorPanel::drawVec3Control(const std::string &label, glm::vec3 &values, float resetValue, float columnWidth) {
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

    void InspectorPanel::inspectGameObject(OnSelectGameObjectEvent *event) {
        this->gameObject = event->getGameObject();
    }

}
