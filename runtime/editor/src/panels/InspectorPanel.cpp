#include "InspectorPanel.hpp"

#include "scene/CameraSystem.hpp"
#include "scene/ScenesManager.hpp"
#include "components/Components.hpp"
#include "components/ComponentsFactory.hpp"

#include "physics/PhysicsEngine.hpp"
#include "resources/ResourcesManager.hpp"

#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel(SystemsRegistry& editorSystemRegistry,
                                   SystemsRegistry& appSystemRegistry,
                                   const std::vector<Entity>& selectedEntities) : m_appSystemRegistry(appSystemRegistry),
                                                                                  m_selectedEntities(selectedEntities),
                                                                                  Panel(editorSystemRegistry) {
    }

    void InspectorPanel::onInit() {
    }

    void InspectorPanel::onUpdate() {
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
                    Mesh& mesh = m_appSystemRegistry.getSystem<ResourcesManager>().getDefaultMesh();
                    Material& material = m_appSystemRegistry.getSystem<ResourcesManager>().getDefaultMaterial();
                    addComponent<MeshRenderer>(mesh, material);
                }
                if (ImGui::BeginMenu("Physics")) {
                    if (ImGui::BeginMenu("Bodies")) {
                        if (ImGui::MenuItem("Static Body")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<StaticBody>(entity, ComponentsFactory::createStaticBody(physicsEngine, tag, transform));
                            }
                        }
                        if (ImGui::MenuItem("Kinematic Body")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<KinematicBody>(entity, ComponentsFactory::createKinematicBody(physicsEngine, tag, transform));
                            }
                        }
                        if (ImGui::MenuItem("Rigid Body")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<RigidBody>(entity, ComponentsFactory::createRigidBody(physicsEngine, tag, transform));
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Colliders")) {
                        if (ImGui::MenuItem("Box Collider")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<BoxCollider>(entity, ComponentsFactory::createBoxCollider(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                            }
                        }
                        if (ImGui::MenuItem("Sphere Collider")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<SphereCollider>(entity, ComponentsFactory::createSphereCollider(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                            }
                        }
                        if (ImGui::MenuItem("Capsule Collider")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<CapsuleCollider>(entity, ComponentsFactory::createCapsuleCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                            }
                        }
                        if (ImGui::MenuItem("Cylinder Collider")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<CylinderCollider>(entity, ComponentsFactory::createCylinderCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                            }
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Triggers")) {
                        if (ImGui::MenuItem("Box Trigger")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<BoxTrigger>(entity, ComponentsFactory::createBoxTrigger(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                            }
                        }

                        if (ImGui::MenuItem("Sphere Trigger")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<SphereTrigger>(entity, ComponentsFactory::createSphereTrigger(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                            }
                        }

                        if (ImGui::MenuItem("Capsule Trigger")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<CapsuleTrigger>(entity, ComponentsFactory::createCapsuleTrigger(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                            }
                        }
                        if (ImGui::MenuItem("Cylinder Trigger")) {
                            Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                            for (const Entity& entity: m_selectedEntities) {
                                Tag& tag = scene.getComponent<Tag>(entity);
                                Transform& transform = scene.getComponent<Transform>(entity);
                                PhysicsEngine& physicsEngine = m_appSystemRegistry.getSystem<PhysicsEngine>();
                                scene.addComponent<CylinderTrigger>(entity, ComponentsFactory::createCylinderTrigger(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                            }
                        }

                        ImGui::EndMenu();
                    }
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
        Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
        std::vector<ComponentTypeID> componentsToDraw = scene.getCommonComponents(m_selectedEntities);

        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<Transform>()) != componentsToDraw.end()) {
            drawComponent<Transform>(firstEntity, "Transform", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 position = component.m_position;
                glm::vec3 rotation = component.m_rotation;
                glm::vec3 scale = component.m_scale;
                
                bool drawPosition = true;
                bool drawOrientation = true;
                bool drawScale = true;

                for (Entity entity: m_selectedEntities) {
                    if (scene.getComponent<Transform>(entity).m_position != position) {
                        drawPosition = false;
                    }
                    if (scene.getComponent<Transform>(entity).m_rotation != rotation) {
                        drawOrientation = false;
                    }
                    if (scene.getComponent<Transform>(entity).m_scale != scale) {
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
                    if (move || position != component.m_position) {
                        component.translateTo(position);
                        m_appSystemRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                        move = true;
                    }
                    if (rotate || rotation != component.m_rotation) {
                        component.setRotation(rotation);
                        m_appSystemRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                        rotate = true;
                    }
                    if (scaling || scale != component.m_scale) {
                        component.setScale(scale);
                        if (scene.hasComponent<BoxCollider>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<BoxCollider>(entity).center,
                                false);
                        }
                        if (scene.hasComponent<SphereCollider>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<SphereCollider>(entity).center,
                                true);
                        }
                        if (scene.hasComponent<CapsuleCollider>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<CapsuleCollider>(entity).center,
                                true);
                        }
                        if (scene.hasComponent<CylinderCollider>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<CylinderCollider>(entity).center,
                                false);
                        }
                        if (scene.hasComponent<BoxTrigger>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<BoxTrigger>(entity).center);
                        }
                        if (scene.hasComponent<SphereTrigger>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<SphereTrigger>(entity).center);
                        }
                        if (scene.hasComponent<CapsuleTrigger>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<CapsuleTrigger>(entity).center);
                        }
                        if (scene.hasComponent<CylinderTrigger>(entity)) {
                            m_appSystemRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                                scene.getComponent<Tag>(entity),
                                scene.getComponent<Transform>(entity),
                                scene.getComponent<CylinderTrigger>(entity).center);
                        }
                        scaling = true;
                    }
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<Camera>()) != componentsToDraw.end()) {
            drawComponent<Camera>(firstEntity, "Camera", [this](auto& component) {
                auto& camera = component;
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                bool isProjectionCommon = true;
                bool isFovCommon = true;
                bool isNearCommon = true;
                bool isFarCommon = true;

                //Camera::ProjectionType commonProjectionType = camera->getProjectionType();
                float commonFov = camera.getFov();
                float commonNear = camera.getNearPlane();
                float commonFar = camera.getFarPlane();


                for (Entity entity: m_selectedEntities) {
                    if (scene.getComponent<Camera>(entity).getFov() != commonFov) {
                        isFovCommon = false;
                    }

                    if (scene.getComponent<Camera>(entity).getNearPlane() != commonNear) {
                        isNearCommon = false;
                    }

                    if (scene.getComponent<Camera>(entity).getFarPlane() != commonFar) {
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
                        bool mainCamera = camera.isMainCamera();

                        CameraSystem& cameraSystem = m_appSystemRegistry.getSystem<CameraSystem>();
                        ImGui::Checkbox("Main Camera", &mainCamera);
                        if (mainCamera) {
                            cameraSystem.setMainCamera(camera);
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
                        if (changeFov) camera.fov.x = commonFov;
                        if (changeNear) cameraSystem.setNear(entity, commonNear);
                        if (changeFar) cameraSystem.setFar(entity, commonFar);
                        //if (changeProjection) cameraSystem.changeProjectionType(entity, commonProjectionType);
                    }
                }

                const char* orthoSizeLabel = isFovCommon ? "Size" : "-";
                const char* orthoNearLabel = isNearCommon ? "Near" : "-";
                const char* orthoFarLabel = isFarCommon ? "Far" : "-";
                /*if (camera->getProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC) {
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
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<MeshRenderer>()) != componentsToDraw.end()) {
            drawComponent<MeshRenderer>(firstEntity, "Mesh Renderer", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                bool isMeshCommon = true;
                bool isMaterialCommon = true;
                const std::string commonMeshName = component.mesh.getName();
                const std::string commonMaterialName = component.material.getName();

                for (Entity entity: m_selectedEntities) {
                    auto currentMeshRenderer = scene.getComponent<MeshRenderer>(entity);

                    if (currentMeshRenderer.mesh.getName() != commonMeshName) {
                        isMeshCommon = false;
                    }

                    if (currentMeshRenderer.material.getName() != commonMaterialName) {
                        isMaterialCommon = false;
                    }
                }

                /*
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
                        /*for (GameObject* entity: m_selectedEntities) {
                            entity->getComponent<MeshRenderer>()->changeMesh(m_appSystemRegistry.getSystem<MeshManager>().getMesh(current_item));
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
                        for (Entity entity: m_selectedEntities) {
                            //entity->getComponent<MeshRenderer>()->changeMaterial(m_appSystemRegistry.getSystem<MaterialManager>().getMaterial(materialName));
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
                        for (Entity entity: m_selectedEntities) {
                            //entity->getComponent<MeshRenderer>()->changeMaterial(m_appSystemRegistry.getSystem<MaterialManager>().getMaterial(materialName));
                        }
                        return;
                    }
                    ImGui::EndDragDropTarget();
                }
                */
                component.paintMesh();
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<StaticBody>()) != componentsToDraw.end()) {
            drawComponent<StaticBody>(firstEntity, "Static Body", [this](auto& component) {
                                          Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                          bool isMassCommon = true;
                                          bool isDensityCommon = true;
                                      }, [this]() {
                                          Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                          for (Entity entity: m_selectedEntities) {
                                              m_appSystemRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                          }
                                      });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<KinematicBody>()) != componentsToDraw.end()) {
            drawComponent<KinematicBody>(firstEntity, "Kinematic Body", [this](auto& component) {
                                         }, [this]() {
                                             Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                             for (Entity entity: m_selectedEntities) {
                                                 m_appSystemRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                             }
                                         });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<RigidBody>()) != componentsToDraw.end()) {
            drawComponent<RigidBody>(firstEntity, "Rigid Body", [this](auto& component) {
                                         Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                         bool isMassCommon = true;
                                         bool isDensityCommon = true;
                                     }, [this]() {
                                         Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                         for (Entity entity: m_selectedEntities) {
                                             m_appSystemRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                         }
                                     });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<BoxCollider>()) != componentsToDraw.end()) {
            drawComponent<BoxCollider>(firstEntity, "Box Collider", [this](auto& component) {
                                           Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                           glm::vec3 commonCenter = component.center;
                                           glm::vec3 commonSize = component.size;
                                           bool isCenterCommon = true;
                                           bool isSizeCommon = true;

                                           for (Entity entity: m_selectedEntities) {
                                               auto currentBoxCollider = scene.getComponent<BoxCollider>(entity);
                                               if (currentBoxCollider.center != commonCenter) {
                                                   isCenterCommon = false;
                                               }
                                               if (currentBoxCollider.size != commonSize) {
                                                   isSizeCommon = false;
                                               }
                                           }

                                           bool changeCenter = false;
                                           bool changeSize = false;
                                           ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                                           ImGuiUtils::drawVec3Control("Scale", commonSize, 1.0f, 100.0f, 0, 0, isSizeCommon);
                                           if (commonCenter != component.center) {
                                               changeCenter = true;
                                           }
                                           if (commonSize != component.size) {
                                               changeSize = true;
                                           }

                                           for (Entity entity: m_selectedEntities) {
                                               if (changeCenter) {
                                                   scene.getComponent<BoxCollider>(entity).center = commonCenter;
                                                   m_appSystemRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                                               }
                                               if (changeSize) {
                                                   scene.getComponent<BoxCollider>(entity).size = commonSize;
                                                   m_appSystemRegistry.getSystem<PhysicsEngine>().resizeCollider(
                                                       Shape::Cube,
                                                       scene.getComponent<Tag>(entity),
                                                       scene.getComponent<Transform>(entity),
                                                       commonCenter,
                                                       commonSize);
                                               }
                                           }
                                       }, [this]() {
                                           Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                           for (Entity entity: m_selectedEntities) {
                                               m_appSystemRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                           }
                                       });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<SphereCollider>()) != componentsToDraw.end()) {
            drawComponent<SphereCollider>(firstEntity, "Sphere Collider", [this](auto& component) {
                                              Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                              glm::vec3 commonCenter = component.center;
                                              float commonRadius = component.radius;
                                              bool isRadiusCommon = true;
                                              bool isCenterCommon = true;

                                              for (Entity entity: m_selectedEntities) {
                                                  auto currentSphereCollider = scene.getComponent<SphereCollider>(entity);
                                                  if (currentSphereCollider.center != commonCenter) {
                                                      isCenterCommon = false;
                                                  }
                                                  if (currentSphereCollider.radius != commonRadius) {
                                                      isRadiusCommon = false;
                                                  }
                                              }

                                              bool changeCenter = false;
                                              bool changeRadius = false;
                                              ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                                              if (commonCenter != component.center) {
                                                  changeCenter = true;
                                              }
                                              if (isRadiusCommon) {
                                                  ImGui::Text("Radius");
                                                  ImGui::SameLine();
                                                  ImGui::DragFloat("##X", &commonRadius, 0.1f, 0.1f,FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                                  if (commonRadius != component.radius) {
                                                      changeRadius = true;
                                                  }
                                              }
                                              for (Entity entity: m_selectedEntities) {
                                                  if (changeCenter) {
                                                      scene.getComponent<SphereCollider>(entity).center = commonCenter;
                                                      m_appSystemRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                                                  }
                                                  if (changeRadius) {
                                                      scene.getComponent<SphereCollider>(entity).radius = commonRadius;
                                                      m_appSystemRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Sphere,
                                                                                                                    scene.getComponent<Tag>(entity),
                                                                                                                    scene.getComponent<Transform>(entity),
                                                                                                                    commonCenter,
                                                                                                                    glm::vec3(commonRadius));
                                                  }
                                              }
                                          }
                                          ,
                                          [this]() {
                                              Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                              for (Entity entity: m_selectedEntities) {
                                                  m_appSystemRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                              }
                                          }
            );
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<CapsuleCollider>()) != componentsToDraw.end()) {
            drawComponent<CapsuleCollider>(firstEntity, "Capsule Collider", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 commonCenter = component.center;
                float commonRadius = component.radius;
                float commonHeight = component.height;

                bool isRadiusCommon = true;
                bool isCenterCommon = true;
                bool isHeightCommon = true;

                for (Entity entity: m_selectedEntities) {
                    auto currentCapsuleCollider = scene.getComponent<CapsuleCollider>(entity);

                    if (currentCapsuleCollider.center != commonCenter) {
                        isCenterCommon = false;
                    }
                    if (currentCapsuleCollider.radius != commonRadius) {
                        isRadiusCommon = false;
                    }
                    if (currentCapsuleCollider.height != commonHeight) {
                        isHeightCommon = false;
                    }
                }

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                if (commonCenter != component.center) {
                    changeCenter = true;
                }

                if (isRadiusCommon) {
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Radius", &commonRadius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                    if (commonRadius != component.radius) {
                        changeRadius = true;
                    }
                }
                if (isHeightCommon) {
                    ImGui::Text("Height");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Height", &commonHeight, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                    if (commonHeight != component.height) {
                        changeHeight = true;
                    }
                }
                for (Entity entity: m_selectedEntities) {
                    if (changeCenter) {
                        scene.getComponent<CapsuleCollider>(entity).center = commonCenter;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                    }
                    if (changeRadius || changeHeight) {
                        scene.getComponent<CapsuleCollider>(entity).radius = commonRadius;
                        scene.getComponent<CapsuleCollider>(entity).height = commonHeight;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Capsule,
                                                                                      scene.getComponent<Tag>(entity),
                                                                                      scene.getComponent<Transform>(entity),
                                                                                      commonCenter,
                                                                                      glm::vec3(commonRadius, commonHeight, commonRadius));
                    }
                }
            }, [this] {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                for (Entity entity: m_selectedEntities) {
                    m_appSystemRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<CylinderCollider>()) != componentsToDraw.end()) {
            drawComponent<CylinderCollider>(firstEntity, "Cylinder Collider", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 commonCenter = component.center;
                float commonHeight = component.height;
                float commonRadius = component.radius;

                bool isRadiusCommon = true;
                bool isCenterCommon = true;
                bool isHeightCommon = true;

                for (Entity entity: m_selectedEntities) {
                    auto currentCylinderCollider = scene.getComponent<CylinderCollider>(entity);

                    if (currentCylinderCollider.center != commonCenter) {
                        isCenterCommon = false;
                    }
                    if (currentCylinderCollider.radius != commonRadius) {
                        isRadiusCommon = false;
                    }
                    if (currentCylinderCollider.height != commonHeight) {
                        isHeightCommon = false;
                    }
                }

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                if (commonCenter != component.center) {
                    changeCenter = true;
                }

                if (isRadiusCommon) {
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Radius", &commonRadius, 0.1f);
                    if (commonRadius != component.radius) {
                        changeRadius = true;
                    }
                }
                if (isHeightCommon) {
                    ImGui::Text("Height");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Height", &commonHeight, 0.1f);
                    if (commonHeight != component.height) {
                        changeHeight = true;
                    }
                }
                for (Entity entity: m_selectedEntities) {
                    if (changeCenter) {
                        scene.getComponent<CylinderCollider>(entity).center = commonCenter;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                    }
                    if (changeRadius || changeHeight) {
                        scene.getComponent<CylinderCollider>(entity).radius = commonRadius;
                        scene.getComponent<CylinderCollider>(entity).height = commonHeight;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Cylinder,
                                                                                      scene.getComponent<Tag>(entity),
                                                                                      scene.getComponent<Transform>(entity),
                                                                                      commonCenter,
                                                                                      glm::vec3(commonRadius, commonHeight, commonRadius));
                    }
                }
            }, [this] {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                for (Entity entity: m_selectedEntities) {
                    m_appSystemRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<BoxTrigger>()) != componentsToDraw.end()) {
            drawComponent<BoxTrigger>(firstEntity, "Box Trigger", [this](auto& component) {
                                          Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                          glm::vec3 commonCenter = component.center;
                                          glm::vec3 commonSize = component.size;
                                          bool isCenterCommon = true;
                                          bool isSizeCommon = true;

                                          for (Entity entity: m_selectedEntities) {
                                              auto currentBoxCollider = scene.getComponent<BoxTrigger>(entity);
                                              if (currentBoxCollider.center != commonCenter) {
                                                  isCenterCommon = false;
                                              }
                                              if (currentBoxCollider.size != commonSize) {
                                                  isSizeCommon = false;
                                              }
                                          }

                                          bool changeCenter = false;
                                          bool changeSize = false;
                                          ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                                          ImGuiUtils::drawVec3Control("Scale", commonSize, 1.0f, 100.0f, 0, 0, isSizeCommon);
                                          if (commonCenter != component.center) {
                                              changeCenter = true;
                                          }
                                          if (commonSize != component.size) {
                                              changeSize = true;
                                          }

                                          for (Entity entity: m_selectedEntities) {
                                              if (changeCenter) {
                                                  scene.getComponent<BoxTrigger>(entity).center = commonCenter;
                                                  m_appSystemRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), commonCenter);
                                              }
                                              if (changeSize) {
                                                  scene.getComponent<BoxTrigger>(entity).size = commonSize;
                                                  m_appSystemRegistry.getSystem<PhysicsEngine>().resizeTrigger(
                                                      Shape::Cube,
                                                      scene.getComponent<Tag>(entity),
                                                      scene.getComponent<Transform>(entity),
                                                      commonCenter,
                                                      commonSize);
                                              }
                                          }
                                      }, [this]() {
                                          Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                          for (Entity entity: m_selectedEntities) {
                                              m_appSystemRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                          }
                                      });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<SphereTrigger>()) != componentsToDraw.end()) {
            drawComponent<SphereTrigger>(firstEntity, "Sphere Trigger", [this](auto& component) {
                                             Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                             glm::vec3 commonCenter = component.center;
                                             float commonRadius = component.radius;
                                             bool isRadiusCommon = true;
                                             bool isCenterCommon = true;

                                             for (Entity entity: m_selectedEntities) {
                                                 auto currentSphereCollider = scene.getComponent<SphereTrigger>(entity);
                                                 if (currentSphereCollider.center != commonCenter) {
                                                     isCenterCommon = false;
                                                 }
                                                 if (currentSphereCollider.radius != commonRadius) {
                                                     isRadiusCommon = false;
                                                 }
                                             }

                                             bool changeCenter = false;
                                             bool changeRadius = false;
                                             ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                                             if (commonCenter != component.center) {
                                                 changeCenter = true;
                                             }
                                             if (isRadiusCommon) {
                                                 ImGui::Text("Radius");
                                                 ImGui::SameLine();
                                                 ImGui::DragFloat("##X", &commonRadius, 0.1f, 0.1f,FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                                 if (commonRadius != component.radius) {
                                                     changeRadius = true;
                                                 }
                                             }
                                             for (Entity entity: m_selectedEntities) {
                                                 if (changeCenter) {
                                                     scene.getComponent<SphereTrigger>(entity).center = commonCenter;
                                                     m_appSystemRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), commonCenter);
                                                 }
                                                 if (changeRadius) {
                                                     scene.getComponent<SphereTrigger>(entity).radius = commonRadius;
                                                     m_appSystemRegistry.getSystem<PhysicsEngine>().resizeTrigger(
                                                         Shape::Sphere,
                                                         scene.getComponent<Tag>(entity),
                                                         scene.getComponent<Transform>(entity),
                                                         commonCenter,
                                                         glm::vec3(commonRadius));
                                                 }
                                             }
                                         }
                                         ,
                                         [this]() {
                                             Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                                             for (Entity entity: m_selectedEntities) {
                                                 m_appSystemRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                             }
                                         }
            );
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<CapsuleTrigger>()) != componentsToDraw.end()) {
            drawComponent<CapsuleTrigger>(firstEntity, "Capsule Trigger", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 commonCenter = component.center;
                float commonRadius = component.radius;
                float commonHeight = component.height;

                bool isRadiusCommon = true;
                bool isCenterCommon = true;
                bool isHeightCommon = true;

                for (Entity entity: m_selectedEntities) {
                    auto currentCapsuleCollider = scene.getComponent<CapsuleTrigger>(entity);

                    if (currentCapsuleCollider.center != commonCenter) {
                        isCenterCommon = false;
                    }
                    if (currentCapsuleCollider.radius != commonRadius) {
                        isRadiusCommon = false;
                    }
                    if (currentCapsuleCollider.height != commonHeight) {
                        isHeightCommon = false;
                    }
                }

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                if (commonCenter != component.center) {
                    changeCenter = true;
                }

                if (isRadiusCommon) {
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Radius", &commonRadius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                    if (commonRadius != component.radius) {
                        changeRadius = true;
                    }
                }
                if (isHeightCommon) {
                    ImGui::Text("Height");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Height", &commonHeight, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                    if (commonHeight != component.height) {
                        changeHeight = true;
                    }
                }
                for (Entity entity: m_selectedEntities) {
                    if (changeCenter) {
                        scene.getComponent<CapsuleTrigger>(entity).center = commonCenter;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), commonCenter);
                    }
                    if (changeRadius || changeHeight) {
                        scene.getComponent<CapsuleTrigger>(entity).radius = commonRadius;
                        scene.getComponent<CapsuleTrigger>(entity).height = commonHeight;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Capsule,
                                                                                     scene.getComponent<Tag>(entity),
                                                                                     scene.getComponent<Transform>(entity),
                                                                                     commonCenter,
                                                                                     glm::vec3(commonRadius, commonHeight, commonRadius));
                    }
                }
            }, [this] {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                for (Entity entity: m_selectedEntities) {
                    m_appSystemRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType::get<CylinderTrigger>()) != componentsToDraw.end()) {
            drawComponent<CylinderTrigger>(firstEntity, "Cylinder Trigger", [this](auto& component) {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 commonCenter = component.center;
                float commonHeight = component.height;
                float commonRadius = component.radius;

                bool isRadiusCommon = true;
                bool isCenterCommon = true;
                bool isHeightCommon = true;

                for (Entity entity: m_selectedEntities) {
                    auto currentCylinderCollider = scene.getComponent<CylinderTrigger>(entity);

                    if (currentCylinderCollider.center != commonCenter) {
                        isCenterCommon = false;
                    }
                    if (currentCylinderCollider.radius != commonRadius) {
                        isRadiusCommon = false;
                    }
                    if (currentCylinderCollider.height != commonHeight) {
                        isHeightCommon = false;
                    }
                }

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0, isCenterCommon);
                if (commonCenter != component.center) {
                    changeCenter = true;
                }

                if (isRadiusCommon) {
                    ImGui::Text("Radius");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Radius", &commonRadius, 0.1f);
                    if (commonRadius != component.radius) {
                        changeRadius = true;
                    }
                }
                if (isHeightCommon) {
                    ImGui::Text("Height");
                    ImGui::SameLine();
                    ImGui::DragFloat("##Height", &commonHeight, 0.1f);
                    if (commonHeight != component.height) {
                        changeHeight = true;
                    }
                }
                for (Entity entity: m_selectedEntities) {
                    if (changeCenter) {
                        scene.getComponent<CylinderTrigger>(entity).center = commonCenter;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), commonCenter);
                    }
                    if (changeRadius || changeHeight) {
                        scene.getComponent<CylinderTrigger>(entity).radius = commonRadius;
                        scene.getComponent<CylinderTrigger>(entity).height = commonHeight;
                        m_appSystemRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Cylinder,
                                                                                     scene.getComponent<Tag>(entity),
                                                                                     scene.getComponent<Transform>(entity),
                                                                                     commonCenter,
                                                                                     glm::vec3(commonRadius, commonHeight, commonRadius));
                    }
                }
            }, [this] {
                Scene& scene = m_appSystemRegistry.getSystem<ScenesManager>().getActiveScene();
                for (Entity entity: m_selectedEntities) {
                    m_appSystemRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                }
            });
        }
    }
}
