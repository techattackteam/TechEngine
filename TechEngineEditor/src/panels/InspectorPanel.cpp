#include <imgui_internal.h>
#include "InspectorPanel.hpp"
#include "components/CameraComponent.hpp"
#include "components/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "PanelsManager.hpp"
#include "components/physics/SphereCollider.hpp"
#include "mesh/CubeMesh.hpp"
#include "mesh/SphereMesh.hpp"
#include "mesh/CylinderMesh.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "material/MaterialManager.hpp"

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
                if (ImGui::MenuItem("Rigid Body")) {
                    PanelsManager::getInstance().getSelectedGameObject()->addComponent<RigidBody>();
                    PanelsManager::getInstance().getSelectedGameObject()->getComponent<RigidBody>()->registerRB();
                }
                if (ImGui::BeginMenu("Colliders")) {
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
            ImGuiUtils::drawVec3Control("Translation", position);
            ImGuiUtils::drawVec3Control("Rotation", orientation);
            ImGuiUtils::drawVec3Control("Scale", scale, 1.0f);
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
            Material &material = meshRenderer->getMaterial();
            if (mesh.getName() == "ImportedMesh") {
                ImGui::Text("Imported Mesh");
            } else {
                static const char *current_item;
                const char *items[] = {"Cube", "Sphere", "Cylinder", "Capsule", "Plane"};
                for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
                    if (mesh.getName() == items[i]) {
                        current_item = items[i];
                        break;
                    }
                }

                if (ImGui::BeginCombo("##combo", current_item)) {// The second parameter is the label previewed before opening the combo.
                    for (auto &item: items) {
                        bool is_selected = (current_item == item); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(item, is_selected))
                            current_item = item;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
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
            }
            static bool open = false;
            if (ImGui::Button(material.getName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0)) && !open) {
                open = true;
                OPENFILENAMEA ofn;
                CHAR szFile[260] = {0};
                CHAR currentDir[256] = {0};
                ZeroMemory(&ofn, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile);
                if (GetCurrentDirectoryA(256, currentDir))
                    ofn.lpstrInitialDir = currentDir;
                ofn.lpstrFilter = ".mat";
                ofn.nFilterIndex = 1;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(&ofn) == TRUE) {
                    open = false;
                    std::string filepath = ofn.lpstrFile;
                    std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);
                    std::string materialName = filename.substr(0, filename.find_last_of("."));
                    meshRenderer->changeMaterial(MaterialManager::getMaterial(materialName));
                }
            };
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    std::string filename = (const char *) payload->Data;
                    std::string extension = filename.substr(filename.find_last_of('.'));
                    if (extension != ".mat")
                        return;
                    std::string materialName = filename.substr(0, filename.find_last_of("."));
                    meshRenderer->changeMaterial(MaterialManager::getMaterial(materialName));
                    return;
                }
                ImGui::EndDragDropTarget();
            }

            component->paintMesh();
        });

        drawComponent<RigidBody>("Rigid Body", [this](auto &component) {
            auto &rigidBody = component;
            float mass = rigidBody->getMass();
            float density = rigidBody->getDensity();
            ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGui::DragFloat("Density", &density, 0.1f, 0.1f, 100.0f, "%.2f");
            if (mass != rigidBody->getMass())
                rigidBody->setMass(mass);
            if (density != rigidBody->getDensity())
                rigidBody->setDensity(density);
        });

        drawComponent<BoxColliderComponent>("Box Collider", [this](auto &component) {
            auto &boxCollider = component;
            glm::vec3 size = boxCollider->getSize();
            glm::vec3 offset = boxCollider->getOffset();
            ImGuiUtils::drawVec3Control("Size", size, 1.0f, 100.0f, 0);
            ImGuiUtils::drawVec3Control("Offset", offset, 0, 100.0f, 0);
            if (size != boxCollider->getSize())
                boxCollider->setSize(size);
            if (offset != boxCollider->getOffset())
                boxCollider->setOffset(offset);
        });

        drawComponent<SphereCollider>("Sphere Collider", [this](auto &component) {
            auto &collider = component;
            float radius = collider->getRadius();
            glm::vec3 offset = collider->getOffset();
            ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGuiUtils::drawVec3Control("Offset", offset, 0, 100.0f, 0);
            if (radius != collider->getRadius() && radius > 0.0f)
                collider->setRadius(radius);
            if (offset != collider->getOffset())
                collider->setOffset(offset);
        });

        drawComponent<CylinderCollider>("Cylinder Collider", [this](auto &component) {
            auto &collider = component;
            float radius = collider->getRadius();
            float height = collider->getHeight();
            glm::vec3 offset = collider->getOffset();
            ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGui::DragFloat("Height", &height, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGuiUtils::drawVec3Control("Offset", offset, 0, 100.0f, 0);
            if (radius != collider->getRadius())
                collider->setRadius(radius);
            if (height != collider->getHeight())
                collider->setHeight(height);
            if (offset != collider->getOffset())
                collider->setOffset(offset);
        });
    }
}
