#include <imgui_internal.h>
#include "InspectorPanel.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "components/CameraComponent.hpp"
#include "components/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "PanelsManager.hpp"
#include "components/physics/SphereCollider.hpp"
#include "mesh/CubeMesh.hpp"
#include "mesh/SphereMesh.hpp"
#include "mesh/CylinderMesh.hpp"
#include "components/physics/CylinderCollider.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel() : Panel("Inspector") {

    }

    void InspectorPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        if (PanelsManager::getInstance().getSelectedGameObject() != nullptr) {
            drawComponents();
        }
        if (ImGui::BeginPopupContextWindow("Add Component", 1)) {
            if (ImGui::MenuItem("Camera")) {
                PanelsManager::getInstance().getSelectedGameObject()->addComponent<CameraComponent>();
            }
            if (ImGui::MenuItem("Mesh Renderer")) {
                PanelsManager::getInstance().getSelectedGameObject()->addComponent<MeshRendererComponent>();
            }
            if (ImGui::BeginMenu("Physics")) {
                if (ImGui::MenuItem("Box Collider")) {
                    PanelsManager::getInstance().getSelectedGameObject()->addComponent<BoxColliderComponent>();
                }
                if (ImGui::MenuItem("Sphere Collider")) {
                    PanelsManager::getInstance().getSelectedGameObject()->addComponent<SphereCollider>();
                }
                if (ImGui::MenuItem("Cylinder Collider")) {
                    PanelsManager::getInstance().getSelectedGameObject()->addComponent<CylinderCollider>();
                }
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
        ImGui::End();
    }

    template<typename T, typename UIFunction>
    void InspectorPanel::drawComponent(const std::string &name, UIFunction uiFunction) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (PanelsManager::getInstance().getSelectedGameObject()->hasComponent<T>()) {
            auto component = PanelsManager::getInstance().getSelectedGameObject()->getComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void *) typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
            ImGui::PopStyleVar();
            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("-", ImVec2{lineHeight, lineHeight})) {
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
                PanelsManager::getInstance().getSelectedGameObject()->removeComponent<T>();
        }
    }

    void InspectorPanel::drawComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("Add Component");


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
                component->setRotation(orientation);
            if (scale != component->scale)
                component->setScale(scale);
        });

        drawComponent<CameraComponent>("Camera", [](auto &component) {
                                           auto &camera = component;

                                           ImGui::Checkbox("Primary", &component->mainCamera);

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

                                           if (camera->getProjectionType() == CameraComponent::ProjectionType::PERSPECTIVE) {
                                               float perspectiveVerticalFov = camera->getFov();
                                               if (ImGui::DragFloat("Vertical FOV", &perspectiveVerticalFov))
                                                   camera->setFov(perspectiveVerticalFov);

                                               float perspectiveNear = camera->getNear();
                                               if (ImGui::DragFloat("Near", &perspectiveNear))
                                                   camera->setNear(perspectiveNear);

                                               float perspectiveFar = camera->getFar();
                                               if (ImGui::DragFloat("Far", &perspectiveFar))
                                                   camera->setFar(perspectiveFar);
                                           }

                                           if (camera->getProjectionType() == CameraComponent::ProjectionType::ORTHOGRAPHIC) {
                                               float orthoSize = camera->getOrthoSize();
                                               if (ImGui::DragFloat("Size", &orthoSize))
                                                   camera->setOrthoSize(orthoSize);

                                               float perspectiveNear = camera->getNear();
                                               if (ImGui::DragFloat("Near", &perspectiveNear))
                                                   camera->setNear(perspectiveNear);

                                               float perspectiveFar = camera->getFar();
                                               if (ImGui::DragFloat("Far", &perspectiveFar))
                                                   camera->setFar(perspectiveFar);

                                           }
                                       }
        );
        drawComponent<MeshRendererComponent>("Mesh Renderer", [this](auto &component) {
            auto &meshRenderer = component;
            auto &mesh = meshRenderer->getMesh();
            auto &material = meshRenderer->getMaterial();
            static const char *current_item;
            const char *items[] = {"Cube", "Sphere", "Cylinder", "Capsule", "Plane"};
            if (mesh.getName() == "Cube") {
                current_item = items[0];
            } else if (mesh.getName() == "Sphere") {
                current_item = items[1];
            } else if (mesh.getName() == "Cylinder") {
                current_item = items[2];
            } else if (mesh.getName() == "Capsule") {
                current_item = items[3];
            } else if (mesh.getName() == "Plane") {
                current_item = items[4];
            } else {
                current_item = items[0];
            }

            if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                    bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable(items[n], is_selected))
                        current_item = items[n];
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
                ImGui::EndCombo();
            }
            if (current_item != mesh.getName()) {
                if (current_item == items[0]) {
                    meshRenderer->changeMesh(new CubeMesh());
                } else if (current_item == items[1]) {
                    meshRenderer->changeMesh(new SphereMesh());
                } else if (current_item == items[2]) {
                    meshRenderer->changeMesh(new CylinderMesh());
                } else if (current_item == items[3]) {
                }
            }
            ImGui::ColorEdit4("Color", glm::value_ptr(material.getColor()));
            drawVec3Control("Ambient", material.getAmbient(), 1, 100.0f, 0, 1);
            drawVec3Control("diffuse", material.getDiffuse(), 1, 100.0f, 0, 1);
            drawVec3Control("specular", material.getSpecular(), 1, 100.0f, 0, 1);
            //component->paintMesh();
        });

        drawComponent<BoxColliderComponent>("Box Collider", [this](auto &component) {
            auto &boxCollider = component;
            glm::vec3 size = boxCollider->getSize();
            glm::vec3 offset = boxCollider->getOffset();
            bool dynamic = boxCollider->isDynamic();
            drawVec3Control("Size", size, 1, 100.0f, 0);
            drawVec3Control("Offset", offset, 1, 100.0f, 0);
            ImGui::Checkbox("Dynamic", &dynamic);
            if (size != boxCollider->getSize())
                boxCollider->setSize(size);
            if (offset != boxCollider->getOffset())
                boxCollider->setOffset(offset);
            if (dynamic != boxCollider->isDynamic())
                boxCollider->setDynamic(dynamic);
        });

        drawComponent<SphereCollider>("Sphere Collider", [this](auto &component) {
            auto &collider = component;
            float radius = collider->getRadius();
            glm::vec3 offset = collider->getOffset();
            bool dynamic = collider->isDynamic();
            ImGui::DragFloat("Radius", &radius, 0.1f, 1, 100.0f, "%.2f");
            drawVec3Control("Offset", offset, 1, 100.0f, 0);
            ImGui::Checkbox("Dynamic", &dynamic);
            if (radius != collider->getRadius())
                collider->setRadius(radius);
            if (offset != collider->getOffset())
                collider->setOffset(offset);
            if (dynamic != collider->isDynamic())
                collider->setDynamic(dynamic);
        });

        drawComponent<CylinderCollider>("Cylinder Collider", [this](auto &component) {
            auto &collider = component;
            float radius = collider->getRadius();
            float height = collider->getHeight();
            glm::vec3 offset = collider->getOffset();
            bool dynamic = collider->isDynamic();
            ImGui::DragFloat("Radius", &radius, 0.1f, 0.1, 100.0f, "%.2f");
            ImGui::DragFloat("Height", &height, 0.1f, 0.1, 100.0f, "%.2f");
            drawVec3Control("Offset", offset, 1, 100.0f, 0);
            ImGui::Checkbox("Dynamic", &dynamic);
            if (radius != collider->getRadius())
                collider->setRadius(radius);
            if (height != collider->getHeight())
                collider->setHeight(height);
            if (offset != collider->getOffset())
                collider->setOffset(offset);
            if (dynamic != collider->isDynamic())
                collider->setDynamic(dynamic);

        });
    }

    void InspectorPanel::drawVec3Control(const std::string &label, glm::vec3 &values, float resetValue, float columnWidth, float min, float max) {
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
        ImGui::DragFloat("##X", &values.x, 0.1f, min, max, "%.2f");
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
        ImGui::DragFloat("##Y", &values.y, 0.1f, min, max, "%.2f");
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
        ImGui::DragFloat("##Z", &values.z, 0.1f, min, max, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }
}
