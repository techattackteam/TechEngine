#include "InspectorPanel.hpp"

#include "scene/CameraSystem.hpp"
#include "scene/Scene.hpp"
#include "scene/TransformSystem.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel(SystemsRegistry& systemRegistry,
                                   SystemsRegistry& appSystemRegistry,
                                   const std::vector<Entity>& selectedEntities) : m_systemRegistry(systemRegistry),
                                                                                  m_appSystemRegistry(appSystemRegistry),
                                                                                  m_selectedEntities(selectedEntities) {
    }

    void InspectorPanel::onInit() {
    }

    void InspectorPanel::onUpdate() {
        Scene& scene = m_appSystemRegistry.getSystem<Scene>();
        if (!m_selectedEntities.empty()) {
            if (m_selectedEntities.size() == 1) {
                //drawComponents();
            } else {
            }
            drawCommonComponents();
            if (ImGui::BeginPopupContextWindow("Add Component", 1)) {
                if (ImGui::MenuItem("Camera")) {
                    addComponent<Camera>();
                }
                if (ImGui::MenuItem("Mesh Renderer")) {
                    //addComponent<MeshRendererComponent>();
                }
                if (ImGui::BeginMenu("Physics")) {
                    /*if (ImGui::MenuItem("Rigid Body")) {
                        addComponent<RigidBody>();
                        for (GameObject* entity: m_selectedEntities) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().addRigidBody(entity->getComponent<RigidBody>());
                        }
                    }
                    if (ImGui::BeginMenu("Colliders")) {
                        if (ImGui::MenuItem("Box Collider")) {
                            addComponent<BoxColliderComponent>();
                            for (GameObject* entity: m_selectedEntities) {
                                m_appSystemRegistry.getSystem<PhysicsEngine>().addCollider(entity->getComponent<BoxColliderComponent>());
                            }
                        }
                        if (ImGui::MenuItem("Sphere Collider")) {
                            addComponent<SphereCollider>();
                            for (GameObject* entity: m_selectedEntities) {
                                m_appSystemRegistry.getSystem<PhysicsEngine>().addCollider(entity->getComponent<SphereCollider>());
                            }
                        }
                        if (ImGui::MenuItem("Cylinder Collider")) {
                            addComponent<CylinderCollider>();
                            for (GameObject* entity: m_selectedEntities) {
                                m_appSystemRegistry.getSystem<PhysicsEngine>().addCollider(entity->getComponent<CylinderCollider>());
                            }
                        }
                        ImGui::EndMenu();
                    }*/
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Network")) {
                    /*if (ImGui::MenuItem("Network Handler")) {
                        addComponent<NetworkSync>();
                    }*/
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }
    }

    void InspectorPanel::drawCommonComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("Add Component");
        }

        ImGui::PopItemWidth();

        Entity firstEntity = m_selectedEntities.front();
        Scene& scene = m_appSystemRegistry.getSystem<Scene>();
        std::vector<std::string> componentsToDraw = scene.getCommonComponents(m_selectedEntities);
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(Tag).name()) != componentsToDraw.end()) {
            drawComponent<Transform>(firstEntity, "Transform", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<Scene>();
                glm::vec3 position = component.position;
                glm::vec3 rotation = component.rotation;
                glm::vec3 scale = component.scale;
                bool drawPosition = true;
                bool drawOrientation = true;
                bool drawScale = true;

                for (Entity entity: m_selectedEntities) {
                    if (scene.getComponent<Transform>(entity).position != position) {
                        drawPosition = false;
                    }
                    if (scene.getComponent<Transform>(entity).rotation != rotation) {
                        drawOrientation = false;
                    }
                    if (scene.getComponent<Transform>(entity).scale != scale) {
                        drawScale = false;
                    }
                }
                ImGuiUtils::drawVec3Control("Translation", position, 0.0f, 100.0f, 0, 0, drawPosition);
                ImGuiUtils::drawVec3Control("Rotation", rotation, 0.0f, 100.0f, 0, 0, drawOrientation);
                ImGuiUtils::drawVec3Control("Scale", scale, 1.0f, 100.0f, 0, 0, drawScale);
                bool move = false;
                bool rotate = false;
                bool scaling = false;
                for (Entity entity: m_selectedEntities) {
                    if (move || position != component.position) {
                        m_appSystemRegistry.getSystem<TransformSystem>().translateTo(entity, position);
                        move = true;
                    }
                    if (rotate || rotation != component.rotation) {
                        m_appSystemRegistry.getSystem<TransformSystem>().setRotation(entity, rotation);
                        rotate = true;
                    }
                    if (scaling || scale != component.scale) {
                        m_appSystemRegistry.getSystem<TransformSystem>().setScale(entity, scale);
                        scaling = true;
                    }
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(Camera).name()) != componentsToDraw.end()) {
            drawComponent<Camera>(firstEntity, "Camera", [this](auto& component) {
                auto& camera = component;
                Scene& scene = m_appSystemRegistry.getSystem<Scene>();
                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                bool isProjectionCommon = true;
                bool isFovCommon = true;
                bool isNearCommon = true;
                bool isFarCommon = true;

                //Camera::ProjectionType commonProjectionType = camera->getProjectionType();
                float commonFov = camera.fov;
                float commonNear = camera.nearPlane;
                float commonFar = camera.farPlane;


                for (Entity entity: m_selectedEntities) {
                    if (scene.getComponent<Camera>(entity).fov != commonFov) {
                        isFovCommon = false;
                    }

                    if (scene.getComponent<Camera>(entity).nearPlane != commonNear) {
                        isNearCommon = false;
                    }

                    if (scene.getComponent<Camera>(entity).farPlane != commonFar) {
                        isFarCommon = false;
                    }
                }
                /*
                const char* currentProjectionTypeString = projectionTypeStrings[(int)camera->getProjectionType()];
                bool changeProjection = false;
                if (ImGui::BeginCombo("Projection", isProjectionCommon ? currentProjectionTypeString : "-")) {
                    for (int i = 0; i < 2; i++) {
                        bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                        if (ImGui::Selectable(projectionTypeStrings[i], isSelected)) {
                            currentProjectionTypeString = projectionTypeStrings[i];
                            commonProjectionType = (Camera::ProjectionType)i;
                            changeProjection = true;
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }*/
                const char* fovLabel = isFovCommon ? "Vertical FOV" : "-";
                const char* nearLabel = isNearCommon ? "Near" : "-";
                const char* farLabel = isFarCommon ? "Far" : "-";
                if (/*camera->getProjectionType() == Camera::ProjectionType::PERSPECTIVE*/ true) {
                    bool changeFov = false;
                    bool changeNear = false;
                    bool changeFar = false;
                    if (m_selectedEntities.size() == 1) {
                        bool isMainCamera = camera.isMainCamera;

                        ImGui::Checkbox("Main Camera", &isMainCamera);
                        if (isMainCamera != camera.isMainCamera) {
                            CameraSystem& cameraSystem = m_appSystemRegistry.getSystem<CameraSystem>();
                            cameraSystem.setMainCamera(m_selectedEntities[0]);
                        }
                    }
                    if (ImGui::DragFloat(fovLabel, &commonFov, 0.1f)) {
                        changeFov = true;
                    }
                    if (ImGui::DragFloat(nearLabel, &commonNear, 0.1f)) {
                        changeNear = true;
                    }
                    if (ImGui::DragFloat(farLabel, &commonFar, 0.1f)) {
                        changeFar = true;
                    }

                    for (Entity entity: m_selectedEntities) {
                        CameraSystem& cameraSystem = m_appSystemRegistry.getSystem<CameraSystem>();
                        if (changeFov) cameraSystem.setFov(entity, commonFov);
                        if (changeNear) cameraSystem.setNear(entity, commonNear);
                        if (changeFar) cameraSystem.setFar(entity, commonFar);
                        //if (changeProjection) cameraSystem.changeProjectionType(entity, commonProjectionType);
                    }
                }

                /*const char* orthoSizeLabel = isFovCommon ? "Size" : "-";
                const char* orthoNearLabel = isNearCommon ? "Near" : "-";
                const char* orthoFarLabel = isFarCommon ? "Far" : "-";
                if (camera->getProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC) {
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

                    for (GameObject* entity: m_selectedEntities) {
                        if (changeOrthoSize) entity->getComponent<Camera>()->setOrthoSize(commonFov);
                        if (changeOrthoNear) entity->getComponent<Camera>()->setNear(commonNear);
                        if (changeOrthoFar) entity->getComponent<Camera>()->setFar(commonFar);
                        if (changeProjection) entity->getComponent<Camera>()->changeProjectionType(commonProjectionType);
                    }
                }*/
            });
        } /*
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(MeshRendererComponent).name()) != componentsToDraw.end()) {
            drawComponent<MeshRendererComponent>(firstEntity, "Mesh Renderer", [this](auto& component) {
                bool isMeshCommon = true;
                bool isMaterialCommon = true;
                std::string commonMeshName = component->getMesh().getName();
                std::string commonMaterialName = component->getMaterial().getName();

                for (GameObject* entity: m_selectedEntities) {
                    auto currentMeshRenderer = entity->getComponent<MeshRendererComponent>();

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
                        for (GameObject* entity: m_selectedEntities) {
                            entity->getComponent<MeshRendererComponent>()->changeMesh(m_appSystemRegistry.getSystem<MeshManager>().getMesh(current_item));
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
                        for (GameObject* entity: m_selectedEntities) {
                            entity->getComponent<MeshRendererComponent>()->changeMaterial(m_appSystemRegistry.getSystem<MaterialManager>().getMaterial(materialName));
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
                        for (GameObject* entity: m_selectedEntities) {
                            entity->getComponent<MeshRendererComponent>()->changeMaterial(m_appSystemRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                        }
                        return;
                    }
                    ImGui::EndDragDropTarget();
                }

                component->paintMesh();
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(RigidBody).name()) != componentsToDraw.end()) {
            drawComponent<RigidBody>(firstEntity, "Rigid Body", [this](auto& component) {
                float commonMass = component->getMass();
                float commonDensity = component->getDensity();
                bool isMassCommon = true;
                bool isDensityCommon = true;

                for (GameObject* entity: m_selectedEntities) {
                    auto currentRigidBody = entity->getComponent<RigidBody>();

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
                for (GameObject* entity: m_selectedEntities) {
                    if (changeMass) entity->getComponent<RigidBody>()->setMass(commonMass);
                    if (changeDensity) entity->getComponent<RigidBody>()->setDensity(commonDensity);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(BoxColliderComponent).name()) != componentsToDraw.end()) {
            drawComponent<BoxColliderComponent>(firstEntity, "Box Collider", [this](auto& component) {
                glm::vec3 commonSize = component->getSize();
                glm::vec3 commonOffset = component->getOffset();
                bool isSizeCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* entity: m_selectedEntities) {
                    auto currentBoxCollider = entity->getComponent<BoxColliderComponent>();

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
                for (GameObject* entity: m_selectedEntities) {
                    if (changeSize) entity->getComponent<BoxColliderComponent>()->setSize(commonSize);
                    if (changeOffset) entity->getComponent<BoxColliderComponent>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(SphereCollider).name()) != componentsToDraw.end()) {
            drawComponent<SphereCollider>(firstEntity, "Sphere Collider", [this](auto& component) {
                float commonRadius = component->getRadius();
                glm::vec3 commonOffset = component->getOffset();

                bool isRadiusCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* entity: m_selectedEntities) {
                    auto currentSphereCollider = entity->getComponent<SphereCollider>();

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
                for (GameObject* entity: m_selectedEntities) {
                    if (changeRadius) entity->getComponent<SphereCollider>()->setRadius(commonRadius);
                    if (changeOffset) entity->getComponent<SphereCollider>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(CylinderCollider).name()) != componentsToDraw.end()) {
            drawComponent<CylinderCollider>(firstEntity, "Cylinder Collider", [this](auto& component) {
                float commonRadius = component->getRadius();
                float commonHeight = component->getHeight();
                glm::vec3 commonOffset = component->getOffset();

                bool isRadiusCommon = true;
                bool isHeightCommon = true;
                bool isOffsetCommon = true;

                for (GameObject* entity: m_selectedEntities) {
                    auto currentCylinderCollider = entity->getComponent<CylinderCollider>();

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

                for (GameObject* entity: m_selectedEntities) {
                    if (changeRadius) entity->getComponent<CylinderCollider>()->setRadius(commonRadius);
                    if (changeHeight) entity->getComponent<CylinderCollider>()->setHeight(commonHeight);
                    if (changeOffset) entity->getComponent<CylinderCollider>()->setOffset(commonOffset);
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), typeid(NetworkSync).name()) != componentsToDraw.end()) {
            bool draw = true;
            for (GameObject* entity: m_selectedEntities) {
                if (!entity->hasComponent<NetworkSync>()) {
                    draw = false;
                    break;
                }
            }
            if (draw) {
                drawComponent<NetworkSync>(firstEntity, "Network Sync", [](auto& component) {
                    ImGui::Text("GameObject is synchronized over network");
                });
            }
        }*/
    }
}
