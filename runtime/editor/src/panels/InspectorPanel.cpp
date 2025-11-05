#include "InspectorPanel.hpp"

#include "scene/CameraSystem.hpp"
#include "scene/ScenesManager.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"

#include "physics/PhysicsEngine.hpp"
#include "renderer/Renderer.hpp"
#include "resources/ResourcesManager.hpp"
#include "scene/SceneInternal.hpp"
#include "ui/InputTextWidget.hpp"
#include "ui/InteractableWidget.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/TextWidget.hpp"
#include "ui/WidgetsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"

#include <imgui_stdlib.h>

namespace TechEngine {
    InspectorPanel::InspectorPanel(SystemsRegistry& editorSystemRegistry,
                                   SystemsRegistry& appSystemRegistry,
                                   HierarchyNode& selectedNode) : m_appSystemsRegistry(appSystemRegistry),
                                                                  m_selectedNode(selectedNode),
                                                                  Panel(editorSystemRegistry) {
    }

    void InspectorPanel::onInit() {
    }

    void InspectorPanel::onUpdate() {
        if (m_selectedNode.type == HierarchyNode::NodeType::Entity) {
            drawComponents();
            openAddComponentMenu();
        } else if (m_selectedNode.type == HierarchyNode::NodeType::Widget) {
            drawWidgetProperties();
        } else if (m_selectedNode.type == HierarchyNode::NodeType::RenderPass) {
            drawRenderPassProperties();
        }
    }

    void InspectorPanel::drawComponents() {
        ImGui::SameLine();
        ImGui::PushItemWidth(-1);

        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("Add Component");
        }

        ImGui::PopItemWidth();

        Entity entity = m_selectedNode.entity;
        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
        std::vector<ComponentTypeID> componentsToDraw = scene.getInternal()->getCommonComponents({entity});

        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<Transform>::get()) != componentsToDraw.end()) {
            drawComponent<Transform>(entity, "Transform", [this, entity](auto& component) {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 position = component.m_position;
                glm::vec3 rotation = component.m_rotation;
                glm::vec3 scale = component.m_scale;

                bool drawPosition = true;
                bool drawOrientation = true;
                bool drawScale = true;

                ImGuiUtils::drawVec3Control("Translation", position, 0.0f, 100.0f, 0, 0, drawPosition);
                ImGuiUtils::drawVec3Control("Rotation", rotation, 0.0f, 100.0f, 0, 0, drawOrientation);
                ImGuiUtils::drawVec3Control("Scale", scale, 1.0f, 100.0f, 0, 0, drawScale);

                bool move = false;
                bool rotate = false;
                bool scaling = false;
                if (move || position != component.m_position) {
                    component.translateTo(position);
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                    move = true;
                }
                if (rotate || rotation != component.m_rotation) {
                    component.setRotation(rotation);
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                    rotate = true;
                }
                if (scaling || scale != component.m_scale) {
                    component.setScale(scale);
                    if (scene.hasComponent<BoxCollider>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<BoxCollider>(entity).center,
                            false);
                    }
                    if (scene.hasComponent<SphereCollider>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<SphereCollider>(entity).center,
                            true);
                    }
                    if (scene.hasComponent<CapsuleCollider>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<CapsuleCollider>(entity).center,
                            true);
                    }
                    if (scene.hasComponent<CylinderCollider>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<CylinderCollider>(entity).center,
                            false);
                    }
                    if (scene.hasComponent<BoxTrigger>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<BoxTrigger>(entity).center);
                    }
                    if (scene.hasComponent<SphereTrigger>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<SphereTrigger>(entity).center);
                    }
                    if (scene.hasComponent<CapsuleTrigger>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<CapsuleTrigger>(entity).center);
                    }
                    if (scene.hasComponent<CylinderTrigger>(entity)) {
                        m_appSystemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                            scene.getComponent<Tag>(entity),
                            scene.getComponent<Transform>(entity),
                            scene.getComponent<CylinderTrigger>(entity).center);
                    }
                    scaling = true;
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<Camera>::get()) != componentsToDraw.end()) {
            drawComponent<Camera>(entity, "Camera", [this, entity](auto& component) {
                auto& camera = component;
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                //Camera::ProjectionType commonProjectionType = camera->getProjectionType();
                float fov = camera.getFov();
                float nearPlane = camera.getNearPlane();
                float farPlane = camera.getFarPlane();

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
                const char* fovLabel = "Vertical FOV";
                const char* nearLabel = "Near";
                const char* farLabel = "Far";
                if (/*camera->getProjectionType() == Camera::ProjectionType::PERSPECTIVE*/ true) {
                    bool changeFov = false;
                    bool changeNear = false;
                    bool changeFar = false;
                    bool mainCamera = camera.isMainCamera();

                    CameraSystem& cameraSystem = m_appSystemsRegistry.getSystem<CameraSystem>();
                    ImGui::Checkbox("Main Camera", &mainCamera);
                    if (mainCamera) {
                        cameraSystem.setMainCamera(camera);
                    }
                    if (ImGui::DragFloat(fovLabel, &fov, 0.1f)) {
                        changeFov = true;
                    }
                    if (ImGui::DragFloat(nearLabel, &nearPlane, 0.1f)) {
                        changeNear = true;
                    }
                    if (ImGui::DragFloat(farLabel, &farPlane, 0.1f)) {
                        changeFar = true;
                    }

                    if (changeFov) camera.fov = fov;
                    if (changeNear) cameraSystem.setNear(entity, nearPlane);
                    if (changeFar) cameraSystem.setFar(entity, farPlane);
                    //if (changeProjection) cameraSystem.changeProjectionType(entity, commonProjectionType);
                }

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
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<MeshRenderer>::get()) != componentsToDraw.end()) {
            drawComponent<MeshRenderer>(entity, "Mesh Renderer", [this](auto& component) {
                /*
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


                if (isMeshCommon && commonMeshName == "export3dcoat") {
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
                    component.changeMesh(m_appSystemRegistry.getSystem<ResourcesManager>().getMesh(current_item));
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
                        if (extension == ".mat") {
                            std::string materialName = filename.substr(0, filename.find_last_of("."));
                            component.changeMaterial(m_appSystemRegistry.getSystem<ResourcesManager>().getMaterial(materialName));
                        } else if (extension == ".tesmesh") {
                            std::string meshName = filename.substr(0, filename.find_last_of("."));
                            component.changeMesh(m_appSystemRegistry.getSystem<ResourcesManager>().getMesh(meshName));
                        }
                        return;
                    }
                    ImGui::EndDragDropTarget();
                }
                    */
                //component.paintMesh();
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<PointLight>::get()) != componentsToDraw.end()) {
            drawComponent<PointLight>(entity, "Point Light", [this](auto& component) {
                glm::vec3 color = component.color;
                float intensity = component.intensity;
                float radius = component.radius;

                glm::vec3 newColor = color;
                float newIntensity = intensity;
                float newRadius = radius;
                ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);
                ImGui::DragFloat("Radius", &newRadius, 0.5f, 0.0f);

                if (color != newColor) {
                    component.color = newColor;
                }
                if (intensity != newIntensity) {
                    component.intensity = newIntensity;
                }
                if (radius != newRadius) {
                    component.radius = newRadius;
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<DirectionalLight>::get()) != componentsToDraw.end()) {
            drawComponent<DirectionalLight>(entity, "Directional Light", [this](auto& component) {
                glm::vec3 color = component.color;
                float intensity = component.intensity;

                glm::vec3 newColor = color;
                float newIntensity = intensity;
                ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);

                if (color != newColor) {
                    component.color = newColor;
                }
                if (intensity != newIntensity) {
                    component.intensity = newIntensity;
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<SpotLight>::get()) != componentsToDraw.end()) {
            drawComponent<SpotLight>(entity, "Spot Light", [this](auto& component) {
                glm::vec3 color = component.color;
                float intensity = component.intensity;
                float innercutoff = component.innerCutoff;
                float outercutoff = component.outerCutoff;

                glm::vec3 newColor = color;
                float newIntensity = intensity;
                ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);
                ImGui::DragFloat("Inner Cutoff", &innercutoff, 0.5f, 0.0f, outercutoff);
                ImGui::DragFloat("Outer Cutoff", &outercutoff, 0.5f, innercutoff);

                if (color != newColor) {
                    component.color = newColor;
                }
                if (intensity != newIntensity) {
                    component.intensity = newIntensity;
                }
                if (innercutoff != component.innerCutoff) {
                    component.innerCutoff = innercutoff;
                }
                if (outercutoff != component.outerCutoff) {
                    component.outerCutoff = outercutoff;
                }
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<StaticBody>::get()) != componentsToDraw.end()) {
            drawComponent<StaticBody>(entity, "Static Body", [this](auto& component) {
                                          Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                      }, [this, entity]() {
                                          Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                          m_appSystemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                      });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<KinematicBody>::get()) != componentsToDraw.end()) {
            drawComponent<KinematicBody>(entity, "Kinematic Body", [this](auto& component) {
                                         }, [this, entity]() {
                                             Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                             m_appSystemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                         });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<RigidBody>::get()) != componentsToDraw.end()) {
            drawComponent<RigidBody>(entity, "Rigid Body", [this](auto& component) {
                                         Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                         bool isMassCommon = true;
                                         bool isDensityCommon = true;
                                     }, [this, entity]() {
                                         Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                         m_appSystemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                     });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<BoxCollider>::get()) != componentsToDraw.end()) {
            drawComponent<BoxCollider>(entity, "Box Collider", [this, entity](auto& component) {
                                           Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                           glm::vec3 center = component.center;
                                           glm::vec3 size = component.size;

                                           bool changeCenter = false;
                                           bool changeSize = false;
                                           ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                           ImGuiUtils::drawVec3Control("Scale", size, 1.0f, 100.0f, 0, 0);
                                           if (center != component.center) {
                                               changeCenter = true;
                                           }
                                           if (size != component.size) {
                                               changeSize = true;
                                           }

                                           if (changeCenter) {
                                               scene.getComponent<BoxCollider>(entity).center = center;
                                               m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                                           }
                                           if (changeSize) {
                                               scene.getComponent<BoxCollider>(entity).size = size;
                                               m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeCollider(
                                                   Shape::Cube,
                                                   scene.getComponent<Tag>(entity),
                                                   scene.getComponent<Transform>(entity),
                                                   center,
                                                   size);
                                           }
                                       }, [this, entity]() {
                                           Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                           m_appSystemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                       });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<SphereCollider>::get()) != componentsToDraw.end()) {
            drawComponent<SphereCollider>(entity, "Sphere Collider", [this, entity](auto& component) {
                                              Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                              glm::vec3 center = component.center;
                                              float radius = component.radius;

                                              bool changeCenter = false;
                                              bool changeRadius = false;
                                              ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                              if (center != component.center) {
                                                  changeCenter = true;
                                              }
                                              ImGui::Text("Radius");
                                              ImGui::SameLine();
                                              ImGui::DragFloat("##X", &radius, 0.1f, 0.1f,FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                              if (radius != component.radius) {
                                                  changeRadius = true;
                                              }
                                              if (changeCenter) {
                                                  scene.getComponent<SphereCollider>(entity).center = center;
                                                  m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                                              }
                                              if (changeRadius) {
                                                  scene.getComponent<SphereCollider>(entity).radius = radius;
                                                  m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Sphere,
                                                                                                                 scene.getComponent<Tag>(entity),
                                                                                                                 scene.getComponent<Transform>(entity),
                                                                                                                 center,
                                                                                                                 glm::vec3(radius));
                                              }
                                          },
                                          [this, entity]() {
                                              Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                              m_appSystemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                          }
            );
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<CapsuleCollider>::get()) != componentsToDraw.end()) {
            drawComponent<CapsuleCollider>(entity, "Capsule Collider", [this, entity](auto& component) {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 center = component.center;
                float radius = component.radius;
                float height = component.height;

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                if (center != component.center) {
                    changeCenter = true;
                }

                ImGui::Text("Radius");
                ImGui::SameLine();
                ImGui::DragFloat("##Radius", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                if (radius != component.radius) {
                    changeRadius = true;
                }
                ImGui::Text("Height");
                ImGui::SameLine();
                ImGui::DragFloat("##Height", &height, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                if (height != component.height) {
                    changeHeight = true;
                }
                if (changeCenter) {
                    scene.getComponent<CapsuleCollider>(entity).center = center;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                }
                if (changeRadius || changeHeight) {
                    scene.getComponent<CapsuleCollider>(entity).radius = radius;
                    scene.getComponent<CapsuleCollider>(entity).height = height;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Capsule,
                                                                                   scene.getComponent<Tag>(entity),
                                                                                   scene.getComponent<Transform>(entity),
                                                                                   center,
                                                                                   glm::vec3(radius, height, radius));
                }
            }, [this, entity] {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                m_appSystemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<CylinderCollider>::get()) != componentsToDraw.end()) {
            drawComponent<CylinderCollider>(entity, "Cylinder Collider", [this, entity](auto& component) {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 commonCenter = component.center;
                float commonHeight = component.height;
                float commonRadius = component.radius;

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0);
                if (commonCenter != component.center) {
                    changeCenter = true;
                }

                ImGui::Text("Radius");
                ImGui::SameLine();
                ImGui::DragFloat("##Radius", &commonRadius, 0.1f);
                if (commonRadius != component.radius) {
                    changeRadius = true;
                }
                ImGui::Text("Height");
                ImGui::SameLine();
                ImGui::DragFloat("##Height", &commonHeight, 0.1f);
                if (commonHeight != component.height) {
                    changeHeight = true;
                }
                if (changeCenter) {
                    scene.getComponent<CylinderCollider>(entity).center = commonCenter;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                }
                if (changeRadius || changeHeight) {
                    scene.getComponent<CylinderCollider>(entity).radius = commonRadius;
                    scene.getComponent<CylinderCollider>(entity).height = commonHeight;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Cylinder,
                                                                                   scene.getComponent<Tag>(entity),
                                                                                   scene.getComponent<Transform>(entity),
                                                                                   commonCenter,
                                                                                   glm::vec3(commonRadius, commonHeight, commonRadius));
                }
            }, [this, entity] {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                m_appSystemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<BoxTrigger>::get()) != componentsToDraw.end()) {
            drawComponent<BoxTrigger>(entity, "Box Trigger", [this, entity](auto& component) {
                                          Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                          glm::vec3 center = component.center;
                                          glm::vec3 size = component.size;

                                          bool changeCenter = false;
                                          bool changeSize = false;
                                          ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                          ImGuiUtils::drawVec3Control("Scale", size, 1.0f, 100.0f, 0, 0);
                                          if (center != component.center) {
                                              changeCenter = true;
                                          }
                                          if (size != component.size) {
                                              changeSize = true;
                                          }

                                          if (changeCenter) {
                                              scene.getComponent<BoxTrigger>(entity).center = center;
                                              m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                          }
                                          if (changeSize) {
                                              scene.getComponent<BoxTrigger>(entity).size = size;
                                              m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(
                                                  Shape::Cube,
                                                  scene.getComponent<Tag>(entity),
                                                  scene.getComponent<Transform>(entity),
                                                  center,
                                                  size);
                                          }
                                      }, [this, entity]() {
                                          Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                          m_appSystemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                      });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<SphereTrigger>::get()) != componentsToDraw.end()) {
            drawComponent<SphereTrigger>(entity, "Sphere Trigger", [this, entity](auto& component) {
                                             Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                             glm::vec3 center = component.center;
                                             float radius = component.radius;

                                             bool changeCenter = false;
                                             bool changeRadius = false;
                                             ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                             if (center != component.center) {
                                                 changeCenter = true;
                                             }
                                             ImGui::Text("Radius");
                                             ImGui::SameLine();
                                             ImGui::DragFloat("##X", &radius, 0.1f, 0.1f,FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                             if (radius != component.radius) {
                                                 changeRadius = true;
                                             }
                                             if (changeCenter) {
                                                 scene.getComponent<SphereTrigger>(entity).center = center;
                                                 m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                             }
                                             if (changeRadius) {
                                                 scene.getComponent<SphereTrigger>(entity).radius = radius;
                                                 m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(
                                                     Shape::Sphere,
                                                     scene.getComponent<Tag>(entity),
                                                     scene.getComponent<Transform>(entity),
                                                     center,
                                                     glm::vec3(radius));
                                             }
                                         },
                                         [this, entity]() {
                                             Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                             m_appSystemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                         }
            );
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<CapsuleTrigger>::get()) != componentsToDraw.end()) {
            drawComponent<CapsuleTrigger>(entity, "Capsule Trigger", [this, entity](auto& component) {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 center = component.center;
                float radius = component.radius;
                float height = component.height;

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                if (center != component.center) {
                    changeCenter = true;
                }

                ImGui::Text("Radius");
                ImGui::SameLine();
                ImGui::DragFloat("##Radius", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                if (radius != component.radius) {
                    changeRadius = true;
                }
                ImGui::Text("Height");
                ImGui::SameLine();
                ImGui::DragFloat("##Height", &height, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                if (height != component.height) {
                    changeHeight = true;
                }
                if (changeCenter) {
                    scene.getComponent<CapsuleTrigger>(entity).center = center;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                }
                if (changeRadius || changeHeight) {
                    scene.getComponent<CapsuleTrigger>(entity).radius = radius;
                    scene.getComponent<CapsuleTrigger>(entity).height = height;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Capsule,
                                                                                  scene.getComponent<Tag>(entity),
                                                                                  scene.getComponent<Transform>(entity),
                                                                                  center,
                                                                                  glm::vec3(radius, height, radius));
                }
            }, [this, entity] {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                m_appSystemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<CylinderTrigger>::get()) != componentsToDraw.end()) {
            drawComponent<CylinderTrigger>(entity, "Cylinder Trigger", [this, entity](auto& component) {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                glm::vec3 center = component.center;
                float height = component.height;
                float radius = component.radius;

                bool changeCenter = false;
                bool changeRadius = false;
                bool changeHeight = false;

                ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                if (center != component.center) {
                    changeCenter = true;
                }

                ImGui::Text("Radius");
                ImGui::SameLine();
                ImGui::DragFloat("##Radius", &radius, 0.1f);
                if (radius != component.radius) {
                    changeRadius = true;
                }
                ImGui::Text("Height");
                ImGui::SameLine();
                ImGui::DragFloat("##Height", &height, 0.1f);
                if (height != component.height) {
                    changeHeight = true;
                }
                if (changeCenter) {
                    scene.getComponent<CylinderTrigger>(entity).center = center;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                }
                if (changeRadius || changeHeight) {
                    scene.getComponent<CylinderTrigger>(entity).radius = radius;
                    scene.getComponent<CylinderTrigger>(entity).height = height;
                    m_appSystemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Cylinder,
                                                                                  scene.getComponent<Tag>(entity),
                                                                                  scene.getComponent<Transform>(entity),
                                                                                  center,
                                                                                  glm::vec3(radius, height, radius));
                }
            }, [this, entity] {
                Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                m_appSystemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
            });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<AudioListener>::get()) != componentsToDraw.end()) {
            drawComponent<AudioListener>(entity, "Audio Listener", [](auto& component) {
                                             // No properties to edit for Audio Listener
                                         }, []() {
                                             // No action on remove for Audio Listener
                                         });
        }
        if (std::find(componentsToDraw.begin(), componentsToDraw.end(), ComponentType<AudioEmitter>::get()) != componentsToDraw.end()) {
            drawComponent<AudioEmitter>(entity, "Audio Emitter", [this](auto& component) {
                                            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                                            ImGui::DragFloat("Volume", &component.volume, 0.01f, 0.0f, 1.0f, "%.2f");
                                            ImGui::DragFloat("Pitch", &component.pitch, 0.01f, 0.0f, 1.0f);
                                            ImGui::Checkbox("Loop", &component.loop);
                                        }, []() {
                                        });
        }
    }

    void InspectorPanel::openAddComponentMenu() {
        if (ImGui::BeginPopupContextWindow("Add Component", 1)) {
            Entity entity = m_selectedNode.entity;
            if (ImGui::MenuItem("Camera")) {
                addComponent<Camera>();
            }
            if (ImGui::MenuItem("Mesh Renderer")) {
                Mesh& mesh = m_appSystemsRegistry.getSystem<ResourcesManager>().getDefaultMesh();
                Material& material = m_appSystemsRegistry.getSystem<ResourcesManager>().getDefaultMaterial();
                MeshRenderer& meshRenderer = addComponent<MeshRenderer>();
                /*meshRenderer.changeMaterial(material);
                meshRenderer.changeMesh(mesh);*/
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Point Light")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                    scene.addComponent<PointLight>(entity, ComponentsFactory::createPointLight());
                } else if (ImGui::MenuItem("Directional Light")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                    scene.addComponent<DirectionalLight>(entity, ComponentsFactory::createDirectionalLight());
                } else if (ImGui::MenuItem("SpotLight")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                    scene.addComponent<SpotLight>(entity, ComponentsFactory::createSpotLight());
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Physics")) {
                if (ImGui::BeginMenu("Bodies")) {
                    if (ImGui::MenuItem("Static Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<StaticBody>(entity, ComponentsFactory::createStaticBody(physicsEngine, tag, transform));
                    }
                    if (ImGui::MenuItem("Kinematic Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<KinematicBody>(entity, ComponentsFactory::createKinematicBody(physicsEngine, tag, transform));
                    }
                    if (ImGui::MenuItem("Rigid Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<RigidBody>(entity, ComponentsFactory::createRigidBody(physicsEngine, tag, transform));
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Colliders")) {
                    if (ImGui::MenuItem("Box Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<BoxCollider>(entity, ComponentsFactory::createBoxCollider(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                    }
                    if (ImGui::MenuItem("Sphere Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<SphereCollider>(entity, ComponentsFactory::createSphereCollider(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                    }
                    if (ImGui::MenuItem("Capsule Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CapsuleCollider>(entity, ComponentsFactory::createCapsuleCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    if (ImGui::MenuItem("Cylinder Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CylinderCollider>(entity, ComponentsFactory::createCylinderCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Triggers")) {
                    if (ImGui::MenuItem("Box Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<BoxTrigger>(entity, ComponentsFactory::createBoxTrigger(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                    }

                    if (ImGui::MenuItem("Sphere Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<SphereTrigger>(entity, ComponentsFactory::createSphereTrigger(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                    }

                    if (ImGui::MenuItem("Capsule Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CapsuleTrigger>(entity, ComponentsFactory::createCapsuleTrigger(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    if (ImGui::MenuItem("Cylinder Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CylinderTrigger>(entity, ComponentsFactory::createCylinderTrigger(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Audio")) {
                if (ImGui::MenuItem("Listener")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                    scene.addComponent<AudioListener>(entity, ComponentsFactory::createAudioListener());
                }
                if (ImGui::MenuItem("Emitter")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
                    scene.addComponent<AudioEmitter>(entity, ComponentsFactory::createAudioEmitter());
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Network")) {
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
    }

    void InspectorPanel::drawWidgetProperties() {
        bool changed = false;
        std::shared_ptr<Widget> selectedWidget = m_selectedNode.widget;
        if (ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::PushID("Position");

            // --- ANCHOR PRESET DROPDOWN ---
            const char* anchorPresetNames[] = {
                "Top Left", "Top Center", "Top Right",
                "Middle Left", "Middle Center", "Middle Right",
                "Bottom Left", "Bottom Center", "Bottom Right",
                "Stretch Top", "Stretch Middle", "Stretch Bottom",
                "Stretch Left", "Stretch Center", "Stretch Right",
                "Stretch Fill"
            };
            int currentPreset = static_cast<int>(selectedWidget->m_preset);
            if (ImGui::Combo("Anchor Preset", &currentPreset, anchorPresetNames, IM_ARRAYSIZE(anchorPresetNames))) {
                UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
                glm::vec4 rootScreenRect = {0.0f, 0.0f, (float)uiRenderer.m_screenWidth, (float)uiRenderer.m_screenHeight};
                glm::vec4 parentScreenRect = selectedWidget->m_parent ? selectedWidget->m_parent->m_finalScreenRect : rootScreenRect;
                selectedWidget->changeAnchor(static_cast<Widget::AnchorPreset>(currentPreset), parentScreenRect, uiRenderer.getDpiScale());

                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Determines how this UI element attaches to its parent.\nThis changes which position and size fields are available below.");
            }

            // --- PIVOT SLIDER ---
            if (ImGui::SliderFloat2("Pivot", &selectedWidget->m_pivot.x, 0.0f, 1.0f, "%.2f")) {
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("The element's own origin point for positioning, scaling, and rotation.\n(0, 0) is bottom-left, (0.5, 0.5) is center, (1, 1) is top-right.");
            }

            ImGui::Separator();

            // --- DYNAMIC POSITION & SIZE FIELDS ---
            bool isStretchingX = selectedWidget->m_anchorMax.x - selectedWidget->m_anchorMin.x > 0.001f;
            bool isStretchingY = selectedWidget->m_anchorMax.y - selectedWidget->m_anchorMin.y > 0.001f;

            // --- X-AXIS CONTROLS ---
            ImGui::Text("Horizontal Axis");
            if (isStretchingX) {
                // If stretching horizontally, control Left and Right margins
                changed |= ImGui::DragFloat("Left", &selectedWidget->m_left, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the left anchor to the left edge.");

                changed |= ImGui::DragFloat("Right", &selectedWidget->m_right, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the right anchor to the right edge.");
            } else {
                // If not stretching, control Position X and a fixed Width
                changed |= ImGui::DragFloat("Pos X", &selectedWidget->m_anchoredPosition.x, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Position of the pivot on the X-axis, relative to the anchor point.");

                changed |= ImGui::DragFloat("Width", &selectedWidget->m_size.x, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("The fixed width of the element.");
            }

            // --- Y-AXIS CONTROLS ---
            ImGui::Text("Vertical Axis");
            if (isStretchingY) {
                // If stretching vertically, control Top and Bottom margins
                changed |= ImGui::DragFloat("Top", &selectedWidget->m_top, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the top anchor to the top edge.");

                changed |= ImGui::DragFloat("Bottom", &selectedWidget->m_bottom, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance from the bottom anchor to the bottom edge.");
            } else {
                // If not stretching, control Position Y and a fixed Height
                changed |= ImGui::DragFloat("Pos Y", &selectedWidget->m_anchoredPosition.y, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Position of the pivot on the Y-axis, relative to the anchor point.");

                changed |= ImGui::DragFloat("Height", &selectedWidget->m_size.y, 1.0f);
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("The fixed height of the element.");
            }

            // --- ROTATION Z ---
            if (ImGui::SliderFloat("Rotation Z", &selectedWidget->m_rotationZ, -180.0f, 180.0f, "%.1f°")) {
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Rotation of the element in degrees around the pivot point.\nPositive values rotate clockwise.");
            }

            ImGui::PopID();
        }

        if (changed) {
            m_appSystemsRegistry.getSystem<WidgetsRegistry>().calculateWidgetLayout(selectedWidget);
        }

        ImGui::Separator();
        if (dynamic_cast<PanelWidget*>(selectedWidget.get())) {
            ImGui::Text("Panel:");
            PanelWidget* widget = dynamic_cast<PanelWidget*>(selectedWidget.get());
            const glm::vec4& currentColor = widget->getBackgroundColor();

            float colorsFloat[4] = {
                currentColor.x,
                currentColor.y,
                currentColor.z,
                currentColor.a
            };
            if (ImGui::ColorPicker4("##ColorValue", colorsFloat)) {
                widget->setBackgroundColor({colorsFloat[0], colorsFloat[1], colorsFloat[2], colorsFloat[3]});
            }
        } else if (dynamic_cast<TextWidget*>(selectedWidget.get())) {
            ImGui::Text("Text Content:");
            ImGui::PushID("Text");
            inspectTextWidget(dynamic_cast<TextWidget*>(selectedWidget.get()));
            ImGui::PopID();
        } else if (dynamic_cast<InputTextWidget*>(selectedWidget.get())) {
            ImGui::Text("Input Text:");
            ImGui::PushID("InputText");
            inspectTextWidget(dynamic_cast<InputTextWidget*>(selectedWidget.get()));
            ImGui::PopID();
        } else if (dynamic_cast<InteractableWidget*>(selectedWidget.get())) {
            ImGui::Text("Interactable Widget:");
            InteractableWidget* widget = dynamic_cast<InteractableWidget*>(selectedWidget.get());
            bool clickable = widget->isClickable();
            if (ImGui::Checkbox("Clickable", &clickable)) {
                widget->setClickable(clickable);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("If unchecked, this widget will not respond to user interactions.");
            }
        }
    }

    bool InspectorPanel::resizableInputTextMultiline(const char* label, std::string* text, const ImVec2& initial_size, float min_height) {
        ImGui::PushID(label);

        static std::map<ImGuiID, float> heights;
        const ImGuiID id = ImGui::GetID("##texteditor");

        if (heights.find(id) == heights.end()) {
            heights[id] = initial_size.y > 0 ? initial_size.y : ImGui::GetTextLineHeight() * 4;
        }
        float& height = heights[id];
        bool changed = ImGui::InputTextMultiline("##editor", text, ImVec2(-FLT_MIN, height));

        const float handle_height = 8.0f; // The thickness of our drag handle

        ImGui::InvisibleButton("##resizer", ImVec2(-FLT_MIN, handle_height));
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (ImGui::IsItemActive()) {
            float drag_delta_y = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0.0f).y;

            height += drag_delta_y;

            if (height < min_height) {
                height = min_height;
            }

            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const ImVec2 handle_min = ImGui::GetItemRectMin();
        const ImVec2 handle_max = ImGui::GetItemRectMax();
        const float handle_line_y = handle_min.y + handle_height / 2.0f;
        const float handle_width = ImGui::GetContentRegionAvail().x * 0.25f; // Make the line 25% of the width
        const float handle_center_x = (handle_min.x + handle_max.x) / 2.0f;

        ImU32 color;
        if (ImGui::IsItemActive()) {
            color = ImGui::GetColorU32(ImGuiCol_ButtonActive);
        } else if (ImGui::IsItemHovered()) {
            color = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        } else {
            color = ImGui::GetColorU32(ImGuiCol_Button);
        }

        draw_list->AddLine(
            ImVec2(handle_center_x - handle_width / 2, handle_line_y),
            ImVec2(handle_center_x + handle_width / 2, handle_line_y),
            color,
            2.0f
        );

        ImGui::PopID();
        return changed;
    }

    void InspectorPanel::drawRenderPassProperties() {
        std::string propertyName = *m_selectedNode.renderPassName;
        drawRenderPassProperty(propertyName, [this, propertyName](Renderer& renderer) {
            if (propertyName == "Ambient Occlusion Settings") {
                auto& aoProperties = renderer.getAOProperties();
                ImGui::Checkbox("Enabled", &aoProperties.enabled);
                ImGui::DragFloat("Radius", &aoProperties.radius, 0.1f, 0.1f, 100.0f);
                ImGui::DragFloat("Thickness", &aoProperties.thickness, 0.1f, 0.0f, 10.0f);
                ImGui::DragInt("Direction Count", &aoProperties.directionCount, 1, 1, 64);
                ImGui::DragInt("Steps", &aoProperties.stepsPerDirection, 1, 1, 64);
            } else if (propertyName == "Bloom Settings") {
                auto& bloomProperties = renderer.getBloomProperties();
                ImGui::Checkbox("Enabled", &bloomProperties.enabled);
                ImGui::DragFloat("Intensity", &bloomProperties.intensity, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Threshold", &bloomProperties.threshold, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Knee", &bloomProperties.knee, 0.1f, 0.0f, 10.0f);
            } else if (propertyName == "Chromatic Aberration Settings") {
                auto& chromaticAberration = renderer.getChromaticAberrationProperties();
                ImGui::Checkbox("Enabled", &chromaticAberration.enabled);
                ImGui::DragFloat("Strength", &chromaticAberration.strength, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Offset", &chromaticAberration.offset, 0.01f, 0.0f, 10.0f);
            } else if (propertyName == "Vignette Settings") {
                auto& vignetteProperties = renderer.getVignetteProperties();
                ImGui::Checkbox("Enabled", &vignetteProperties.enabled);
                ImGui::DragFloat("Intensity", &vignetteProperties.strength, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Smoothness", &vignetteProperties.power, 0.1f, 0.0f, 10.0f);
            } else if (propertyName == "Gamma Settings") {
                auto& gammaProperties = renderer.getGammaProperties();
                ImGui::Checkbox("Enabled", &gammaProperties.enabled);
                ImGui::DragFloat("Gamma", &gammaProperties.gamma, 0.1f, 0.1f, 10.0f);
                ImGui::ColorPicker3("Gamma RGB", &gammaProperties.gammaRGB[0]);
                ImGui::DragFloat("Gamma Intensity", &gammaProperties.gammaIntensity, 0.1f, 0.0f, 10.0f);
                ImGui::ColorPicker3("Lift", &gammaProperties.lift[0]);
                ImGui::DragFloat("Lift Intensity", &gammaProperties.liftIntensity, 0.1f, 0.0f, 10.0f);
                ImGui::ColorPicker3("Gain", &gammaProperties.gain[0]);
                ImGui::DragFloat("Gain Intensity", &gammaProperties.gainIntensity, 0.1f, 0.0f, 10.0f);
            } else if (propertyName == "Color Grading Settings") {
                auto& colorGradingProperties = renderer.getColorGradingProperties();
                ImGui::DragFloat("Exposure", &colorGradingProperties.exposure, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Saturation", &colorGradingProperties.saturation, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Contrast", &colorGradingProperties.contrast, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Brightness", &colorGradingProperties.brightness, 0.1f, 0.0f, 10.0f);
                ImGui::Checkbox("Use LUT", &colorGradingProperties.useLUT);
                ImGui::DragFloat("LUT Strength", &colorGradingProperties.lutStrength, 0.1f, 0.0f, 10.0f);
            } else if (propertyName == "Film Grain Settings") {
                auto& filmGrainProperties = renderer.getFilmGrainProperties();
                ImGui::Checkbox("Enabled", &filmGrainProperties.filmGrainEnabled);
                ImGui::DragFloat("Intensity", &filmGrainProperties.filmGrainIntensity, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Size", &filmGrainProperties.filmGrainSize, 0.1f, 0.1f, 10.0f);
            } else if (propertyName == "Fog Settings") {
                auto& fogProperties = renderer.getFogProperties();
                ImGui::Checkbox("Enabled", &fogProperties.enabled);
                ImGui::Separator();
                ImGui::DragFloat("Density", &fogProperties.fogDensity, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("Height Falloff", &fogProperties.fogHeightFalloff, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Height", &fogProperties.fogHeight, 0.1f, -1000.0f, 1000.0f);
                ImGui::Separator();
                ImGui::DragFloat("Start Distance", &fogProperties.fogStart, 0.1f, 0.0f, fogProperties.fogEnd);
                ImGui::DragFloat("End Distance", &fogProperties.fogEnd, 0.1f, fogProperties.fogStart, 1000.0f);
                ImGui::Separator();
                ImGui::DragFloat("Skybox Fog Amount", &fogProperties.skyboxFogAmount, 0.1f, 0.0f, 1.0f);
                const char* blendModes[] = {"Height", "Distance", "Max", "Additive"};
                ImGui::Combo("Blend Mode", &fogProperties.fogBlendMode, blendModes, IM_ARRAYSIZE(blendModes));
                ImGui::Separator();
                ImGui::ColorPicker3("Base Color", &fogProperties.fogColorBase[0]);
                ImGui::ColorPicker3("Sky Color", &fogProperties.fogColorSky[0]);
                ImGui::Separator();
                ImGui::Checkbox("Use Directional Color", &fogProperties.useDirectionalColor);
                ImGui::ColorPicker3("Color Sun", &fogProperties.fogColorSun[0]);
                ImGui::DragFloat("Scattering Intensity", &fogProperties.sunScatteringIntensity, 0.1f, 0.0f, 10.0f);
                ImGui::Separator();
                ImGui::DragFloat("Mie Scattering", &fogProperties.mieScattering, 0.1f, 0.1f, 10.0f);
                ImGui::DragFloat("Rayleigh Scattering", &fogProperties.rayleighScattering, 0.1f, 0.1f, 10.0f);
            } else if ("Volumetric Settings" == propertyName) {
                auto& volumetricSettings = renderer.getVolumetricSettings();
                auto& froxelParams = renderer.getFroxelParams();
                ImGui::Checkbox("Enabled", &volumetricSettings.enabled);
                ImGuiUtils::drawVec3Control("Scattering", volumetricSettings.scatteringCoefficient, 0.10f, 100.0f, 0.0f, 100.0f);
                ImGui::DragFloat("Density", &volumetricSettings.density, 0.01f, 0.0f, 100.0f);
                ImGuiUtils::drawVec3Control("Absorption", volumetricSettings.absorptionCoefficient, 0.0f, 100.0f, 0.0f, 10.0f);
                ImGui::DragFloat("Anisotropy", &volumetricSettings.anisotropy, 0.01f, -1.0f, 1.0f);
                ImGuiUtils::drawVec3Control("Emissive", volumetricSettings.emissiveCoefficient, 0.00f, 100.0f, 0.0f, 100.0f);
                ImGui::DragFloat("Blending Factor", &volumetricSettings.blendingFactor, 0.01f, 0.0f, 1.0f);
                ImGui::Checkbox("My Implementation", &froxelParams.myImplementation);
            }
        });
    }
}
