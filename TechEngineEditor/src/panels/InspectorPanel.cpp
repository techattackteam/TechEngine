#include <imgui_internal.h>
#include "InspectorPanel.hpp"
#include "components/render/CameraComponent.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include "PanelsManager.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "core/Logger.hpp"
#include <windows.h>

#include "components/network/NetworkSync.hpp"
#include "mesh/MeshManager.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel(const std::string& name,
                                   SystemsRegistry& editorRegistry,
                                   SystemsRegistry& appRegistry,
                                   std::vector<GameObject*>& selectedGameObjects) : appRegistry(appRegistry),
                                                                                    selectedGameObjects(selectedGameObjects),
                                                                                    Panel(name, editorRegistry) {
    }

    void InspectorPanel::init() {
        Panel::init();
    }

    void InspectorPanel::onUpdate() {
        if (!selectedGameObjects.empty()) {
            if (selectedGameObjects.size() == 1) {
                drawComponents();
            } else {
                drawCommonComponents();
            }
            if (ImGui::BeginPopupContextWindow("Add Component", 1)) {
                if (ImGui::MenuItem("Camera")) {
                    addComponent<CameraComponent>();
                }
                if (ImGui::MenuItem("Mesh Renderer")) {
                    addComponent<MeshRendererComponent>();
                }
                if (ImGui::BeginMenu("Physics")) {
                    if (ImGui::MenuItem("Rigid Body")) {
                        addComponent<RigidBody>();
                        for (GameObject* gameObject: selectedGameObjects) {
                            appRegistry.getSystem<PhysicsEngine>().addRigidBody(gameObject->getComponent<RigidBody>());
                        }
                    }
                    if (ImGui::BeginMenu("Colliders")) {
                        if (ImGui::MenuItem("Box Collider")) {
                            addComponent<BoxColliderComponent>();
                            for (GameObject* gameObject: selectedGameObjects) {
                                appRegistry.getSystem<PhysicsEngine>().addCollider(gameObject->getComponent<BoxColliderComponent>());
                            }
                        }
                        if (ImGui::MenuItem("Sphere Collider")) {
                            addComponent<SphereCollider>();
                            for (GameObject* gameObject: selectedGameObjects) {
                                appRegistry.getSystem<PhysicsEngine>().addCollider(gameObject->getComponent<SphereCollider>());
                            }
                        }
                        if (ImGui::MenuItem("Cylinder Collider")) {
                            addComponent<CylinderCollider>();
                            for (GameObject* gameObject: selectedGameObjects) {
                                appRegistry.getSystem<PhysicsEngine>().addCollider(gameObject->getComponent<CylinderCollider>());
                            }
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Network")) {
                    if (ImGui::MenuItem("Network Handler")) {
                        addComponent<NetworkSync>();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }
    }

    template<typename T, typename UIFunction>
    void InspectorPanel::drawComponent(GameObject* gameObject, const std::string& name, UIFunction uiFunction) {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (gameObject->hasComponent<T>()) {
            auto component = gameObject->getComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
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

            if (removeComponent) {
                for (GameObject* gameObject: selectedGameObjects) {
                    gameObject->removeComponent<T>();
                }
            }
        }
    }

    void InspectorPanel::drawComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("Add Component");
        }

        ImGui::PopItemWidth();
        GameObject* gameObject = selectedGameObjects.front();
        drawComponent<TransformComponent>(gameObject, "Transform", [this](auto& component) {
            bool update = false;
            glm::vec3 position = component->position;
            glm::vec3 orientation = component->orientation;
            glm::vec3 scale = component->scale;
            ImGuiUtils::drawVec3Control("Translation", position);
            ImGuiUtils::drawVec3Control("Rotation", orientation);
            ImGuiUtils::drawVec3Control("Scale", scale, 1.0f);
            if (position != component->position) {
                component->translateTo(position);
                update = true;
            }
            if (orientation != component->orientation) {
                component->setRotation(orientation);
                update = true;
            }

            if (scale != component->scale) {
                component->setScale(scale);
                update = true;
            }

            if (update && component->getGameObject()->template hasComponent<RigidBody>()) {
                appRegistry.getSystem<PhysicsEngine>().addRigidBody(component->getGameObject()->template getComponent<RigidBody>());
            } else if (update && component->getGameObject()->template hasComponent<BoxColliderComponent>()) {
                appRegistry.getSystem<PhysicsEngine>().addCollider(component->getGameObject()->template getComponent<BoxColliderComponent>());
            } else if (update && component->getGameObject()->template hasComponent<SphereCollider>()) {
                appRegistry.getSystem<PhysicsEngine>().addCollider(component->getGameObject()->template getComponent<SphereCollider>());
            } else if (update && component->getGameObject()->template hasComponent<CylinderCollider>()) {
                appRegistry.getSystem<PhysicsEngine>().addCollider(component->getGameObject()->template getComponent<CylinderCollider>());
            }
        });
        drawComponent<CameraComponent>(gameObject, "Camera", [](auto& component) {
            auto& camera = component;

            ImGui::Checkbox("Primary", &component->mainCamera);

            const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
            const char* currentProjectionTypeString = projectionTypeStrings[(int)camera->getProjectionType()];
            if (ImGui::BeginCombo("Projection", currentProjectionTypeString)) {
                for (int i = 0; i < 2; i++) {
                    bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                    if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
                        currentProjectionTypeString = projectionTypeStrings[i];
                        camera->changeProjectionType((CameraComponent::ProjectionType)i);
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
        });
        drawComponent<MeshRendererComponent>(gameObject, "Mesh Renderer", [this](auto& component) {
            auto& meshRenderer = component;
            auto& mesh = meshRenderer->getMesh();
            Material& material = meshRenderer->getMaterial();
            if (mesh.getName() == "ImportedMesh") {
                ImGui::Text("Imported Mesh");
            } else {
                static const char* current_item;
                const char* items[] = {"Cube", "Sphere", "Cylinder", "Capsule", "Plane"};
                for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
                    if (mesh.getName() == items[i]) {
                        current_item = items[i];
                        break;
                    }
                }

                if (ImGui::BeginCombo("##combo", current_item)) {
                    // The second parameter is the label previewed before opening the combo.
                    for (auto& item: items) {
                        bool is_selected = (current_item == item); // You can store your selection however you want, outside or inside your objects
                        if (ImGui::Selectable(item, is_selected))
                            current_item = item;
                        if (is_selected)
                            ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                    }
                    ImGui::EndCombo();
                }
                if (current_item != mesh.getName()) {
                    meshRenderer->changeMesh(appRegistry.getSystem<MeshManager>().getMesh(current_item));
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
                    meshRenderer->changeMaterial(appRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                }
            };
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    std::string filename = (const char*)payload->Data;
                    std::string extension = filename.substr(filename.find_last_of('.'));
                    if (extension != ".mat")
                        return;
                    std::string materialName = filename.substr(0, filename.find_last_of("."));
                    meshRenderer->changeMaterial(appRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                    return;
                }
                ImGui::EndDragDropTarget();
            }

            component->paintMesh();
        });
        drawComponent<RigidBody>(gameObject, "Rigid Body", [](auto& component) {
            auto& rigidBody = component;
            float mass = rigidBody->getMass();
            float density = rigidBody->getDensity();
            ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGui::DragFloat("Density", &density, 0.1f, 0.1f, 100.0f, "%.2f");
            if (mass != rigidBody->getMass())
                rigidBody->setMass(mass);
            if (density != rigidBody->getDensity())
                rigidBody->setDensity(density);
        });
        drawComponent<BoxColliderComponent>(gameObject, "Box Collider", [](auto& component) {
            auto& boxCollider = component;
            glm::vec3 size = boxCollider->getSize();
            glm::vec3 offset = boxCollider->getOffset();
            ImGuiUtils::drawVec3Control("Size", size, 1.0f, 100.0f, 0);
            ImGuiUtils::drawVec3Control("Offset", offset, 0, 100.0f, 0);
            if (size != boxCollider->getSize())
                boxCollider->setSize(size);
            if (offset != boxCollider->getOffset())
                boxCollider->setOffset(offset);
        });
        drawComponent<SphereCollider>(gameObject, "Sphere Collider", [](auto& component) {
            auto& collider = component;
            float radius = collider->getRadius();
            glm::vec3 offset = collider->getOffset();
            ImGui::DragFloat("Radius", &radius, 0.1f, 0.1f, 100.0f, "%.2f");
            ImGuiUtils::drawVec3Control("Offset", offset, 0, 100.0f, 0);
            if (radius != collider->getRadius() && radius > 0.0f)
                collider->setRadius(radius);
            if (offset != collider->getOffset())
                collider->setOffset(offset);
        });
        drawComponent<CylinderCollider>(gameObject, "Cylinder Collider", [](auto& component) {
            auto& collider = component;
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
        drawComponent<NetworkSync>(gameObject, "Network Sync", [](auto& component) {
            ImGui::Text("GameObject is synchronized over network");
        });
    }

    void InspectorPanel::drawCommonComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("Add Component");
        }

        ImGui::PopItemWidth();
        std::vector<std::string> componentsToDraw = std::vector<std::string>();

        GameObject* firstObject = selectedGameObjects.front();
        for (std::pair<std::string, Component*> component: firstObject->getComponents()) {
            bool isCommon = true;
            for (GameObject* currentObject: selectedGameObjects) {
                if (!currentObject->hasComponent(component.first)) {
                    isCommon = false;
                    break;
                }
            }

            if (isCommon) {
                componentsToDraw.push_back(component.first);
            }
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(TransformComponent).name()) != componentsToDraw.end()) {
            drawComponent<TransformComponent>(firstObject, "Transform", [this](auto& component) {
                glm::vec3 position = component->getPosition();
                glm::vec3 orientation = component->getOrientation();
                glm::vec3 scale = component->getScale();
                bool drawPosition = true;
                bool drawOrientation = true;
                bool drawScale = true;

                for (GameObject* gameObject: selectedGameObjects) {
                    if (gameObject->getComponent<TransformComponent>()->position != position) {
                        drawPosition = false;
                    }
                    if (gameObject->getComponent<TransformComponent>()->orientation != orientation) {
                        drawOrientation = false;
                    }
                    if (gameObject->getComponent<TransformComponent>()->scale != scale) {
                        drawScale = false;
                    }
                }
                ImGuiUtils::drawVec3Control("Translation", position, 0.0f, 100.0f, 0, 0, drawPosition);
                ImGuiUtils::drawVec3Control("Rotation", orientation, 0.0f, 100.0f, 0, 0, drawOrientation);
                ImGuiUtils::drawVec3Control("Scale", scale, 1.0f, 100.0f, 0, 0, drawScale);
                bool move = false;
                bool rotate = false;
                bool scaling = false;
                for (GameObject* gameObject: selectedGameObjects) {
                    if (position != component->position || move) {
                        gameObject->getComponent<TransformComponent>()->translateTo(position);
                        move = true;
                    }
                    if (orientation != component->getOrientation() || rotate) {
                        gameObject->getComponent<TransformComponent>()->setRotation(orientation);
                        rotate = true;
                    }
                    if (scale != component->getScale() || scaling) {
                        gameObject->getComponent<TransformComponent>()->setScale(scale);
                        scaling = true;
                    }
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(CameraComponent).name()) != componentsToDraw.end()) {
            drawComponent<CameraComponent>(firstObject, "Camera", [this](auto& component) {
                auto& camera = component;

                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                bool isProjectionCommon = true;
                bool isFovCommon = true;
                bool isNearCommon = true;
                bool isFarCommon = true;

                CameraComponent::ProjectionType commonProjectionType = camera->getProjectionType();
                float commonFov = camera->getFov();
                float commonNear = camera->getNear();
                float commonFar = camera->getFar();


                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentCamera = gameObject->getComponent<CameraComponent>();

                    if (currentCamera->getProjectionType() != commonProjectionType) {
                        isProjectionCommon = false;
                    }

                    if (currentCamera->getFov() != commonFov) {
                        isFovCommon = false;
                    }

                    if (currentCamera->getNear() != commonNear) {
                        isNearCommon = false;
                    }

                    if (currentCamera->getFar() != commonFar) {
                        isFarCommon = false;
                    }
                }
                const char* currentProjectionTypeString = projectionTypeStrings[(int)camera->getProjectionType()];
                bool changeProjection = false;
                if (ImGui::BeginCombo("Projection", isProjectionCommon ? currentProjectionTypeString : "-")) {
                    for (int i = 0; i < 2; i++) {
                        bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                        if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
                            currentProjectionTypeString = projectionTypeStrings[i];
                            commonProjectionType = (CameraComponent::ProjectionType)i;
                            changeProjection = true;
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }
                const char* fovLabel = isFovCommon ? "Vertical FOV" : "-";
                const char* nearLabel = isNearCommon ? "Near" : "-";
                const char* farLabel = isFarCommon ? "Far" : "-";
                if (camera->getProjectionType() == CameraComponent::ProjectionType::PERSPECTIVE) {
                    bool changeFov = false;
                    bool changeNear = false;
                    bool changeFar = false;
                    if (ImGui::DragFloat(fovLabel, &commonFov, 0.1f)) {
                        changeFov = true;
                    }
                    if (ImGui::DragFloat(nearLabel, &commonNear, 0.1f)) {
                        changeNear = true;
                    }
                    if (ImGui::DragFloat(farLabel, &commonFar, 0.1f)) {
                        changeFar = true;
                    }

                    for (GameObject* gameObject: selectedGameObjects) {
                        if (changeFov) gameObject->getComponent<CameraComponent>()->setFov(commonFov);
                        if (changeNear) gameObject->getComponent<CameraComponent>()->setNear(commonNear);
                        if (changeFar) gameObject->getComponent<CameraComponent>()->setFar(commonFar);
                        if (changeProjection) gameObject->getComponent<CameraComponent>()->changeProjectionType(commonProjectionType);
                    }
                }

                const char* orthoSizeLabel = isFovCommon ? "Size" : "-";
                const char* orthoNearLabel = isNearCommon ? "Near" : "-";
                const char* orthoFarLabel = isFarCommon ? "Far" : "-";
                if (camera->getProjectionType() == CameraComponent::ProjectionType::ORTHOGRAPHIC) {
                    bool changeOrthoSize = false;
                    bool changeOrthoNear = false;
                    bool changeOrthoFar = false;
                    if (ImGui::DragFloat(orthoSizeLabel, &commonFov, 0.1f)) {
                        changeOrthoSize = true;
                    }
                    if (ImGui::DragFloat(orthoNearLabel, &commonNear, 0.1f)) {
                        changeOrthoNear = true;
                    }
                    if (ImGui::DragFloat(orthoFarLabel, &commonFar, 0.1f)) {
                        changeOrthoFar = true;
                    }

                    for (GameObject* gameObject: selectedGameObjects) {
                        if (changeOrthoSize) gameObject->getComponent<CameraComponent>()->setOrthoSize(commonFov);
                        if (changeOrthoNear) gameObject->getComponent<CameraComponent>()->setNear(commonNear);
                        if (changeOrthoFar) gameObject->getComponent<CameraComponent>()->setFar(commonFar);
                        if (changeProjection) gameObject->getComponent<CameraComponent>()->changeProjectionType(commonProjectionType);
                    }
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(MeshRendererComponent).name()) != componentsToDraw.end()) {
            drawComponent<MeshRendererComponent>(firstObject, "Mesh Renderer", [this](auto& component) {
                bool isMeshCommon = true;
                bool isMaterialCommon = true;
                std::string commonMeshName = component->getMesh().getName();
                std::string commonMaterialName = component->getMaterial().getName();

                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentMeshRenderer = gameObject->getComponent<MeshRendererComponent>();

                    if (currentMeshRenderer->getMesh().getName() != commonMeshName) {
                        isMeshCommon = false;
                    }

                    if (currentMeshRenderer->getMaterial().getName() != commonMaterialName) {
                        isMaterialCommon = false;
                    }
                }


                if (isMeshCommon && commonMeshName == "ImportedMesh") {
                    ImGui::Text("Imported Mesh");
                } else {
                    static const char* current_item;
                    const char* items[] = {"Cube", "Sphere", "Cylinder", "Capsule", "Plane"};
                    for (auto& item: items) {
                        if (commonMeshName == item) {
                            current_item = item;
                            break;
                        }
                    }

                    if (ImGui::BeginCombo("##combo", isMeshCommon ? current_item : "-")) {
                        // The second parameter is the label previewed before opening the combo.
                        for (auto& item: items) {
                            bool is_selected = (current_item == item); // You can store your selection however you want, outside or inside your objects
                            if (ImGui::Selectable(item, is_selected))
                                current_item = item;
                            if (is_selected)
                                ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                        }
                        ImGui::EndCombo();
                    }
                    if (current_item != commonMeshName) {
                        for (GameObject* gameObject: selectedGameObjects) {
                            gameObject->getComponent<MeshRendererComponent>()->changeMesh(appRegistry.getSystem<MeshManager>().getMesh(current_item));
                        }
                    }
                }
                static bool open = false;
                if (ImGui::Button(isMaterialCommon ? commonMaterialName.c_str() : "-", ImVec2(ImGui::GetContentRegionAvail().x, 0)) && !open) {
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
                        for (GameObject* gameObject: selectedGameObjects) {
                            gameObject->getComponent<MeshRendererComponent>()->changeMaterial(appRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                        }
                    }
                };
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                        std::string filename = (const char*)payload->Data;
                        std::string extension = filename.substr(filename.find_last_of('.'));
                        if (extension != ".mat")
                            return;
                        std::string materialName = filename.substr(0, filename.find_last_of("."));
                        for (GameObject* gameObject: selectedGameObjects) {
                            gameObject->getComponent<MeshRendererComponent>()->changeMaterial(appRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                        }
                        return;
                    }
                    ImGui::EndDragDropTarget();
                }

                component->paintMesh();
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(RigidBody).name()) != componentsToDraw.end()) {
            drawComponent<RigidBody>(firstObject, "Rigid Body", [this](auto& component) {
                float commonMass = component->getMass();
                float commonDensity = component->getDensity();
                bool isMassCommon = true;
                bool isDensityCommon = true;

                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentRigidBody = gameObject->getComponent<RigidBody>();

                    if (currentRigidBody->getMass() != commonMass) {
                        isMassCommon = false;
                    }

                    if (currentRigidBody->getDensity() != commonDensity) {
                        isDensityCommon = false;
                    }
                }

                bool changeMass = false;
                bool changeDensity = false;
                if (ImGui::DragFloat(isMassCommon ? "Mass" : "-", &commonMass, 0.1f, 0.1f, 100.0f, "%.2f")) {
                    changeMass = true;
                }
                if (ImGui::DragFloat(isDensityCommon ? "Density" : "-", &commonDensity, 0.1f, 0.1f, 100.0f, "%.2f")) {
                    changeDensity = true;
                }
                for (GameObject* gameObject: selectedGameObjects) {
                    if (changeMass) gameObject->getComponent<RigidBody>()->setMass(commonMass);
                    if (changeDensity) gameObject->getComponent<RigidBody>()->setDensity(commonDensity);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(BoxColliderComponent).name()) != componentsToDraw.end()) {
            drawComponent<BoxColliderComponent>(firstObject, "Box Collider", [this](auto& component) {
                glm::vec3 commonSize = component->getSize();
                glm::vec3 commonOffset = component->getOffset();
                bool isSizeCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentBoxCollider = gameObject->getComponent<BoxColliderComponent>();

                    if (currentBoxCollider->getSize() != commonSize) {
                        isSizeCommon = false;
                    }

                    if (currentBoxCollider->getOffset() != commonOffset) {
                        isOffsetCommon = false;
                    }
                }

                bool changeSize = false;
                bool changeOffset = false;
                ImGuiUtils::drawVec3Control("Size", commonSize, 1.0f, 100.0f, 0, 0, isSizeCommon);
                ImGuiUtils::drawVec3Control("Offset", commonOffset, 0, 100.0f, 0, 0, isOffsetCommon);
                if (commonSize != component->getSize()) {
                    changeSize = true;
                }
                if (commonOffset != component->getOffset()) {
                    changeOffset = true;
                }
                for (GameObject* gameObject: selectedGameObjects) {
                    if (changeSize) gameObject->getComponent<BoxColliderComponent>()->setSize(commonSize);
                    if (changeOffset) gameObject->getComponent<BoxColliderComponent>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(SphereCollider).name()) != componentsToDraw.end()) {
            drawComponent<SphereCollider>(firstObject, "Sphere Collider", [this](auto& component) {
                float commonRadius = component->getRadius();
                glm::vec3 commonOffset = component->getOffset();

                bool isRadiusCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentSphereCollider = gameObject->getComponent<SphereCollider>();

                    if (currentSphereCollider->getRadius() != commonRadius) {
                        isRadiusCommon = false;
                    }

                    if (currentSphereCollider->getOffset() != commonOffset) {
                        isOffsetCommon = false;
                    }
                }

                bool changeRadius = false;
                bool changeOffset = false;

                if (ImGui::DragFloat("Radius", &commonRadius, 0.1f, 0.1f, 100.0f, "%.2f")) {
                    changeRadius = true;
                }
                ImGuiUtils::drawVec3Control("Offset", commonOffset, 0, 100.0f, 0, 0, isOffsetCommon);
                if (commonOffset != component->getOffset()) {
                    changeOffset = true;
                }
                for (GameObject* gameObject: selectedGameObjects) {
                    if (changeRadius) gameObject->getComponent<SphereCollider>()->setRadius(commonRadius);
                    if (changeOffset) gameObject->getComponent<SphereCollider>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(CylinderCollider).name()) != componentsToDraw.end()) {
            drawComponent<CylinderCollider>(firstObject, "Cylinder Collider", [this](auto& component) {
                float commonRadius = component->getRadius();
                float commonHeight = component->getHeight();
                glm::vec3 commonOffset = component->getOffset();

                bool isRadiusCommon = true;
                bool isHeightCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* gameObject: selectedGameObjects) {
                    auto currentCylinderCollider = gameObject->getComponent<CylinderCollider>();

                    if (currentCylinderCollider->getRadius() != commonRadius) {
                        isRadiusCommon = false;
                    }

                    if (currentCylinderCollider->getHeight() != commonHeight) {
                        isHeightCommon = false;
                    }

                    if (currentCylinderCollider->getOffset() != commonOffset) {
                        isOffsetCommon = false;
                    }
                }

                bool changeRadius = false;
                bool changeHeight = false;
                bool changeOffset = false;

                if (ImGui::DragFloat("Radius", &commonRadius, 0.1f, 0.1f, 100.0f, "%.2f")) {
                    changeRadius = true;
                }
                if (ImGui::DragFloat("Height", &commonHeight, 0.1f, 0.1f, 100.0f, "%.2f")) {
                    changeHeight = true;
                }
                ImGuiUtils::drawVec3Control("Offset", commonOffset, 0, 100.0f, 0, 0, isOffsetCommon);
                if (commonOffset != component->getOffset()) {
                    changeOffset = true;
                }

                for (GameObject* gameObject: selectedGameObjects) {
                    if (changeRadius) gameObject->getComponent<CylinderCollider>()->setRadius(commonRadius);
                    if (changeHeight) gameObject->getComponent<CylinderCollider>()->setHeight(commonHeight);
                    if (changeOffset) gameObject->getComponent<CylinderCollider>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(NetworkSync).name()) != componentsToDraw.end()) {
            bool draw = true;
            for (GameObject* gameObject: selectedGameObjects) {
                if (!gameObject->hasComponent<NetworkSync>()) {
                    draw = false;
                    break;
                }
            }
            if (draw) {
                drawComponent<NetworkSync>(firstObject, "Network Sync", [](auto& component) {
                    ImGui::Text("GameObject is synchronized over network");
                });
            }
        }
    }

    template<typename C, typename... A>
    void InspectorPanel::addComponent(A&... args) {
        for (GameObject* gameObject: selectedGameObjects) {
            gameObject->addComponent<C>(args...);
        }
    }
}
