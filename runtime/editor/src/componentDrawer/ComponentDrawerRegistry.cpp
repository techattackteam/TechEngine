#include "ComponentDrawerRegistry.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "physics/PhysicsEngine.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/CameraSystem.hpp"
#include "systems/SystemsRegistry.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "TechEngine/core/components/Components.hpp"

namespace TechEngine {
    ComponentDrawerRegistry::ComponentDrawerRegistry(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void ComponentDrawerRegistry::drawAll(Entity entity, Scene& scene, std::function<void(const ComponentDrawer&, Entity, Scene&)> drawFunction) const {
        for (const ComponentDrawer& drawer: m_drawers) {
            if (drawer.hasComponent(entity, scene)) {
                drawFunction(drawer, entity, scene);
            }
        }
    }

    void ComponentDrawerRegistry::registerAllDrawers() {
        registerTransform();
        registerCamera();
        registerMeshRenderer();
        registerPointLight();
        registerDirectionalLight();
        registerSpotLight();
        registerStaticBody();
        registerKinematicBody();
        registerRigidBody();
        registerBoxCollider();
        registerSphereCollider();
        registerCapsuleCollider();
        registerCylinderCollider();
        registerBoxTrigger();
        registerSphereTrigger();
        registerCapsuleTrigger();
        registerCylinderTrigger();
        registerAudioListener();
        registerAudioEmitter();
    }

    void ComponentDrawerRegistry::registerTransform() {
        registerDrawer<Transform>("transform", [this](Transform& component, Entity entity, Scene& scene) {
            glm::vec3 position = component.m_position;
            glm::vec3 rotation = component.m_rotation;
            glm::vec3 scale = component.m_scale;

            const bool drawPosition = true;
            const bool drawOrientation = true;
            const bool drawScale = true;

            ImGuiUtils::drawVec3Control("Translation", position, 0.0f, 100.0f, 0, 0, drawPosition);
            ImGuiUtils::drawVec3Control("Rotation", rotation, 0.0f, 100.0f, 0, 0, drawOrientation);
            ImGuiUtils::drawVec3Control("Scale", scale, 1.0f, 100.0f, 0, 0, drawScale);

            bool move = false;
            bool rotate = false;
            bool scaling = false;
            if (move || position != component.m_position) {
                component.translateTo(position);
                m_systemsRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                move = true;
            }
            if (rotate || rotation != component.m_rotation) {
                component.setRotation(rotation);
                m_systemsRegistry.getSystem<PhysicsEngine>().moveOrRotateBody(scene.getComponent<Tag>(entity), scene.getComponent<Transform>(entity));
                rotate = true;
            }
            if (scaling || scale != component.m_scale) {
                component.setScale(scale);
                if (scene.hasComponent<BoxCollider>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<BoxCollider>(entity).center,
                        false);
                }
                if (scene.hasComponent<SphereCollider>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<SphereCollider>(entity).center,
                        true);
                }
                if (scene.hasComponent<CapsuleCollider>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<CapsuleCollider>(entity).center,
                        true);
                }
                if (scene.hasComponent<CylinderCollider>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleCollider(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<CylinderCollider>(entity).center,
                        false);
                }
                if (scene.hasComponent<BoxTrigger>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<BoxTrigger>(entity).center);
                }
                if (scene.hasComponent<SphereTrigger>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<SphereTrigger>(entity).center);
                }
                if (scene.hasComponent<CapsuleTrigger>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<CapsuleTrigger>(entity).center);
                }
                if (scene.hasComponent<CylinderTrigger>(entity)) {
                    m_systemsRegistry.getSystem<PhysicsEngine>().rescaleTrigger(
                        scene.getComponent<Tag>(entity),
                        scene.getComponent<Transform>(entity),
                        scene.getComponent<CylinderTrigger>(entity).center);
                }
                scaling = true;
            }
        });
    }

    void ComponentDrawerRegistry::registerCamera() {
        registerDrawer<Camera>("Camera", [this](Camera& camera, Entity entity, Scene& scene) {
            float fov = camera.getFov();
            float nearPlane = camera.getNearPlane();
            float farPlane = camera.getFarPlane();

            const char* fovLabel = "Vertical FOV";
            const char* nearLabel = "Near";
            const char* farLabel = "Far";
            if (true) {
                bool changeFov = false;
                bool changeNear = false;
                bool changeFar = false;
                bool mainCamera = camera.isMainCamera();

                CameraSystem& cameraSystem = m_systemsRegistry.getSystem<CameraSystem>();
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

                if (changeFov) {
                    camera.fov = fov;
                }
                if (changeNear) {
                    cameraSystem.setNear(entity, nearPlane);
                }
                if (changeFar) {
                    cameraSystem.setFar(entity, farPlane);
                }
            }
        });
    }

    void ComponentDrawerRegistry::registerMeshRenderer() {
        registerDrawer<MeshRenderer>(
            "Mesh Renderer",
            [this](MeshRenderer& meshRenderer, Entity entity, Scene& scene) {
                ResourceSystem& resourceSystem = m_systemsRegistry.getSystem<ResourceSystem>();

                std::string meshName = resourceSystem.getMeshResource(meshRenderer.meshUUID)->getName();
                std::string materialName = resourceSystem.getMaterialResource(meshRenderer.materialUUID)->getName();

                static float columnMeshWidth = 100.0f;
                static float columnMaterialWidth = 100.0f;

                ImGui::AlignTextToFramePadding(); // Vertically centers text with the button
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnMeshWidth);
                columnMeshWidth = ImGui::GetColumnWidth();
                ImGui::Text("Mesh");
                ImGui::NextColumn();

                ImGui::Button(meshName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                        std::string filename = (const char*)payload->Data;
                        std::string extension = filename.substr(filename.find_last_of('.'));
                        if (extension == ".tesmesh") {
                            std::string newMeshName = filename.substr(0, filename.find_last_of("."));
                            if (false) {
                                TE_LOGGER_WARN("Mesh '{}' is not loaded. Please load it first via Content Browser.", newMeshName);
                            } else {
                                meshRenderer.changeMesh(m_systemsRegistry.getSystem<ResourceSystem>().getMeshResource(newMeshName)->getUUID());
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::EndColumns();

                ImGui::AlignTextToFramePadding();
                ImGui::Columns(2);
                ImGui::SetColumnWidth(0, columnMaterialWidth);
                columnMaterialWidth = ImGui::GetColumnWidth();
                ImGui::Text("Material");
                ImGui::NextColumn();


                ImGui::Button(materialName.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                        std::string filename = (const char*)payload->Data;
                        std::string extension = filename.substr(filename.find_last_of('.'));
                        if (extension == ".mat") {
                            std::string newMaterialName = filename.substr(0, filename.find_last_of("."));
                            meshRenderer.changeMaterial(m_systemsRegistry.getSystem<ResourceSystem>().getMaterialResource(newMaterialName)->getUUID());
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            },
            [](Entity e, Scene& scene) {
                scene.removeComponent<MeshRenderer>(e);
            }
        );
    }

    void ComponentDrawerRegistry::registerPointLight() {
        registerDrawer<PointLight>("Point Light",
                                   [this](PointLight& pointLight, Entity entity, Scene& scene) {
                                       glm::vec3 color = pointLight.color;
                                       float intensity = pointLight.intensity;
                                       float radius = pointLight.radius;

                                       glm::vec3 newColor = color;
                                       float newIntensity = intensity;
                                       float newRadius = radius;
                                       ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                                       ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);
                                       ImGui::DragFloat("Radius", &newRadius, 0.5f, 0.0f);

                                       if (color != newColor) pointLight.color = newColor;
                                       if (intensity != newIntensity) pointLight.intensity = newIntensity;
                                       if (radius != newRadius) pointLight.radius = newRadius;
                                   }
        );
    }

    void ComponentDrawerRegistry::registerDirectionalLight() {
        registerDrawer<DirectionalLight>("Directional Light",
                                         [this](DirectionalLight& directionalLight, Entity entity, Scene& scene) {
                                             glm::vec3 color = directionalLight.color;
                                             float intensity = directionalLight.intensity;

                                             glm::vec3 newColor = color;
                                             float newIntensity = intensity;
                                             ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                                             ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);

                                             if (color != newColor) directionalLight.color = newColor;
                                             if (intensity != newIntensity) directionalLight.intensity = newIntensity;
                                         }
        );
    }

    void ComponentDrawerRegistry::registerSpotLight() {
        registerDrawer<SpotLight>("Spot Light",
                                  [this](SpotLight& spotLight, Entity entity, Scene& scene) {
                                      glm::vec3 color = spotLight.color;
                                      float intensity = spotLight.intensity;
                                      float innercutoff = spotLight.innerCutoff;
                                      float outercutoff = spotLight.outerCutoff;

                                      glm::vec3 newColor = color;
                                      float newIntensity = intensity;
                                      ImGui::ColorEdit3("Color", (float*)&newColor, ImGuiColorEditFlags_Float);
                                      ImGui::DragFloat("Intensity", &newIntensity, 0.5f, 0.0f);
                                      ImGui::DragFloat("Inner Cutoff", &innercutoff, 0.5f, 0.0f, outercutoff);
                                      ImGui::DragFloat("Outer Cutoff", &outercutoff, 0.5f, innercutoff);

                                      if (color != newColor) spotLight.color = newColor;
                                      if (intensity != newIntensity) spotLight.intensity = newIntensity;
                                      if (innercutoff != spotLight.innerCutoff) spotLight.innerCutoff = innercutoff;
                                      if (outercutoff != spotLight.outerCutoff) spotLight.outerCutoff = outercutoff;
                                  }
        );
    }

    void ComponentDrawerRegistry::registerStaticBody() {
        registerDrawer<StaticBody>("Static Body",
                                   [](StaticBody& component, Entity entity, Scene& scene) {
                                   },
                                   [this](Entity entity, Scene& scene) {
                                       m_systemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                       scene.removeComponent<StaticBody>(entity);
                                   }
        );
    }

    void ComponentDrawerRegistry::registerKinematicBody() {
        registerDrawer<KinematicBody>("Kinematic Body",
                                      [](KinematicBody& component, Entity entity, Scene& scene) {
                                      },
                                      [this](Entity entity, Scene& scene) {
                                          m_systemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                          scene.removeComponent<KinematicBody>(entity);
                                      }
        );
    }

    void ComponentDrawerRegistry::registerRigidBody() {
        registerDrawer<RigidBody>("Rigid Body",
                                  [](RigidBody& component, Entity entity, Scene& scene) {
                                      bool isMassCommon = true;
                                      bool isDensityCommon = true;
                                  },
                                  [this](Entity entity, Scene& scene) {
                                      m_systemsRegistry.getSystem<PhysicsEngine>().removeBody(scene.getComponent<Tag>(entity));
                                      scene.removeComponent<RigidBody>(entity);
                                  }
        );
    }

    void ComponentDrawerRegistry::registerBoxCollider() {
        registerDrawer<BoxCollider>("Box Collider",
                                    [this](BoxCollider& boxCollider, Entity entity, Scene& scene) {
                                        glm::vec3 center = boxCollider.center;
                                        glm::vec3 size = boxCollider.size;

                                        bool changeCenter = false;
                                        bool changeSize = false;
                                        ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                        ImGuiUtils::drawVec3Control("Scale", size, 1.0f, 100.0f, 0, 0);
                                        if (center != boxCollider.center) changeCenter = true;
                                        if (size != boxCollider.size) changeSize = true;

                                        if (changeCenter) {
                                            scene.getComponent<BoxCollider>(entity).center = center;
                                            m_systemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                                        }
                                        if (changeSize) {
                                            scene.getComponent<BoxCollider>(entity).size = size;
                                            m_systemsRegistry.getSystem<PhysicsEngine>().resizeCollider(
                                                Shape::Cube, scene.getComponent<Tag>(entity),
                                                scene.getComponent<Transform>(entity), center, size);
                                        }
                                    },
                                    [this](Entity entity, Scene& scene) {
                                        m_systemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                        scene.removeComponent<BoxCollider>(entity);
                                    }
        );
    }

    void ComponentDrawerRegistry::registerSphereCollider() {
        registerDrawer<SphereCollider>("Sphere Collider",
                                       [this](SphereCollider& component, Entity entity, Scene& scene) {
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
                                           ImGui::DragFloat("##X", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                           if (radius != component.radius) {
                                               changeRadius = true;
                                           }
                                           if (changeCenter) {
                                               scene.getComponent<SphereCollider>(entity).center = center;
                                               m_systemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                                           }
                                           if (changeRadius) {
                                               scene.getComponent<SphereCollider>(entity).radius = radius;
                                               m_systemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Sphere,
                                                                                                           scene.getComponent<Tag>(entity),
                                                                                                           scene.getComponent<Transform>(entity),
                                                                                                           center, glm::vec3(radius));
                                           }
                                       },
                                       [this](Entity entity, Scene& scene) {
                                           m_systemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                           scene.removeComponent<SphereCollider>(entity);
                                       }
        );
    }

    void ComponentDrawerRegistry::registerCapsuleCollider() {
        registerDrawer<CapsuleCollider>("Capsule Collider",
                                        [this](CapsuleCollider& component, Entity entity, Scene& scene) {
                                            glm::vec3 center = component.center;
                                            float radius = component.radius;
                                            float height = component.height;

                                            bool changeCenter = false;
                                            bool changeRadius = false;
                                            bool changeHeight = false;

                                            ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                            if (center != component.center) changeCenter = true;

                                            ImGui::Text("Radius");
                                            ImGui::SameLine();
                                            ImGui::DragFloat("##Radius", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                            if (radius != component.radius) changeRadius = true;

                                            ImGui::Text("Height");
                                            ImGui::SameLine();
                                            ImGui::DragFloat("##Height", &height, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                            if (height != component.height) changeHeight = true;

                                            if (changeCenter) {
                                                scene.getComponent<CapsuleCollider>(entity).center = center;
                                                m_systemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), center);
                                            }
                                            if (changeRadius || changeHeight) {
                                                scene.getComponent<CapsuleCollider>(entity).radius = radius;
                                                scene.getComponent<CapsuleCollider>(entity).height = height;
                                                m_systemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Capsule,
                                                                                                            scene.getComponent<Tag>(entity),
                                                                                                            scene.getComponent<Transform>(entity),
                                                                                                            center, glm::vec3(radius, height, radius));
                                            }
                                        },
                                        [this](Entity entity, Scene& scene) {
                                            m_systemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                            scene.removeComponent<CapsuleCollider>(entity);
                                        }
        );
    }

    void ComponentDrawerRegistry::registerCylinderCollider() {
        registerDrawer<CylinderCollider>("Cylinder Collider",
                                         [this](CylinderCollider& component, Entity entity, Scene& scene) {
                                             glm::vec3 commonCenter = component.center;
                                             float commonHeight = component.height;
                                             float commonRadius = component.radius;

                                             bool changeCenter = false;
                                             bool changeRadius = false;
                                             bool changeHeight = false;

                                             ImGuiUtils::drawVec3Control("Center", commonCenter, 0, 100.0f, 0, 0);
                                             if (commonCenter != component.center) changeCenter = true;

                                             ImGui::Text("Radius");
                                             ImGui::SameLine();
                                             ImGui::DragFloat("##Radius", &commonRadius, 0.1f);
                                             if (commonRadius != component.radius) changeRadius = true;

                                             ImGui::Text("Height");
                                             ImGui::SameLine();
                                             ImGui::DragFloat("##Height", &commonHeight, 0.1f);
                                             if (commonHeight != component.height) changeHeight = true;

                                             if (changeCenter) {
                                                 scene.getComponent<CylinderCollider>(entity).center = commonCenter;
                                                 m_systemsRegistry.getSystem<PhysicsEngine>().recenterCollider(scene.getComponent<Tag>(entity), commonCenter);
                                             }
                                             if (changeRadius || changeHeight) {
                                                 scene.getComponent<CylinderCollider>(entity).radius = commonRadius;
                                                 scene.getComponent<CylinderCollider>(entity).height = commonHeight;
                                                 m_systemsRegistry.getSystem<PhysicsEngine>().resizeCollider(Shape::Cylinder,
                                                                                                             scene.getComponent<Tag>(entity),
                                                                                                             scene.getComponent<Transform>(entity),
                                                                                                             commonCenter, glm::vec3(commonRadius, commonHeight, commonRadius));
                                             }
                                         },
                                         [this](Entity entity, Scene& scene) {
                                             m_systemsRegistry.getSystem<PhysicsEngine>().removeCollider(scene.getComponent<Tag>(entity));
                                             scene.removeComponent<CylinderCollider>(entity);
                                         }
        );
    }

    void ComponentDrawerRegistry::registerBoxTrigger() {
        registerDrawer<BoxTrigger>("Box Trigger",
                                   [this](BoxTrigger& component, Entity entity, Scene& scene) {
                                       glm::vec3 center = component.center;
                                       glm::vec3 size = component.size;

                                       bool changeCenter = false;
                                       bool changeSize = false;
                                       ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                       ImGuiUtils::drawVec3Control("Scale", size, 1.0f, 100.0f, 0, 0);
                                       if (center != component.center) changeCenter = true;
                                       if (size != component.size) changeSize = true;

                                       if (changeCenter) {
                                           scene.getComponent<BoxTrigger>(entity).center = center;
                                           m_systemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                       }
                                       if (changeSize) {
                                           scene.getComponent<BoxTrigger>(entity).size = size;
                                           m_systemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Cube,
                                                                                                      scene.getComponent<Tag>(entity),
                                                                                                      scene.getComponent<Transform>(entity),
                                                                                                      center, size);
                                       }
                                   },
                                   [this](Entity entity, Scene& scene) {
                                       m_systemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                       scene.removeComponent<BoxTrigger>(entity);
                                   }
        );
    }

    void ComponentDrawerRegistry::registerSphereTrigger() {
        registerDrawer<SphereTrigger>("Sphere Trigger",
                                      [this](SphereTrigger& component, Entity entity, Scene& scene) {
                                          glm::vec3 center = component.center;
                                          float radius = component.radius;

                                          bool changeCenter = false;
                                          bool changeRadius = false;
                                          ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                          if (center != component.center) changeCenter = true;

                                          ImGui::Text("Radius");
                                          ImGui::SameLine();
                                          ImGui::DragFloat("##X", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                          if (radius != component.radius) changeRadius = true;

                                          if (changeCenter) {
                                              scene.getComponent<SphereTrigger>(entity).center = center;
                                              m_systemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                          }
                                          if (changeRadius) {
                                              scene.getComponent<SphereTrigger>(entity).radius = radius;
                                              m_systemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Sphere,
                                                                                                         scene.getComponent<Tag>(entity),
                                                                                                         scene.getComponent<Transform>(entity),
                                                                                                         center, glm::vec3(radius));
                                          }
                                      },
                                      [this](Entity entity, Scene& scene) {
                                          m_systemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                          scene.removeComponent<SphereTrigger>(entity);
                                      }
        );
    }

    void ComponentDrawerRegistry::registerCapsuleTrigger() {
        registerDrawer<CapsuleTrigger>("Capsule Trigger",
                                       [this](CapsuleTrigger& component, Entity entity, Scene& scene) {
                                           glm::vec3 center = component.center;
                                           float radius = component.radius;
                                           float height = component.height;

                                           bool changeCenter = false;
                                           bool changeRadius = false;
                                           bool changeHeight = false;

                                           ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                           if (center != component.center) changeCenter = true;

                                           ImGui::Text("Radius");
                                           ImGui::SameLine();
                                           ImGui::DragFloat("##Radius", &radius, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                           if (radius != component.radius) changeRadius = true;

                                           ImGui::Text("Height");
                                           ImGui::SameLine();
                                           ImGui::DragFloat("##Height", &height, 0.1f, 0.1f, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                           if (height != component.height) changeHeight = true;

                                           if (changeCenter) {
                                               scene.getComponent<CapsuleTrigger>(entity).center = center;
                                               m_systemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                           }
                                           if (changeRadius || changeHeight) {
                                               scene.getComponent<CapsuleTrigger>(entity).radius = radius;
                                               scene.getComponent<CapsuleTrigger>(entity).height = height;
                                               m_systemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Capsule,
                                                                                                          scene.getComponent<Tag>(entity),
                                                                                                          scene.getComponent<Transform>(entity),
                                                                                                          center, glm::vec3(radius, height, radius));
                                           }
                                       },
                                       [this](Entity entity, Scene& scene) {
                                           m_systemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                           scene.removeComponent<CapsuleTrigger>(entity);
                                       }
        );
    }

    void ComponentDrawerRegistry::registerCylinderTrigger() {
        registerDrawer<CylinderTrigger>("Cylinder Trigger",
                                        [this](CylinderTrigger& component, Entity entity, Scene& scene) {
                                            glm::vec3 center = component.center;
                                            float height = component.height;
                                            float radius = component.radius;

                                            bool changeCenter = false;
                                            bool changeRadius = false;
                                            bool changeHeight = false;

                                            ImGuiUtils::drawVec3Control("Center", center, 0, 100.0f, 0, 0);
                                            if (center != component.center) changeCenter = true;

                                            ImGui::Text("Radius");
                                            ImGui::SameLine();
                                            ImGui::DragFloat("##Radius", &radius, 0.1f);
                                            if (radius != component.radius) changeRadius = true;

                                            ImGui::Text("Height");
                                            ImGui::SameLine();
                                            ImGui::DragFloat("##Height", &height, 0.1f);
                                            if (height != component.height) changeHeight = true;

                                            if (changeCenter) {
                                                scene.getComponent<CylinderTrigger>(entity).center = center;
                                                m_systemsRegistry.getSystem<PhysicsEngine>().recenterTrigger(scene.getComponent<Tag>(entity), center);
                                            }
                                            if (changeRadius || changeHeight) {
                                                scene.getComponent<CylinderTrigger>(entity).radius = radius;
                                                scene.getComponent<CylinderTrigger>(entity).height = height;
                                                m_systemsRegistry.getSystem<PhysicsEngine>().resizeTrigger(Shape::Cylinder,
                                                                                                           scene.getComponent<Tag>(entity),
                                                                                                           scene.getComponent<Transform>(entity),
                                                                                                           center, glm::vec3(radius, height, radius));
                                            }
                                        },
                                        [this](Entity entity, Scene& scene) {
                                            m_systemsRegistry.getSystem<PhysicsEngine>().removeTrigger(scene.getComponent<Tag>(entity));
                                            scene.removeComponent<CylinderTrigger>(entity);
                                        }
        );
    }

    void ComponentDrawerRegistry::registerAudioListener() {
        registerDrawer<AudioListener>("Audio Listener",
                                      [](AudioListener& component, Entity entity, Scene& scene) {
                                          // No properties to edit for Audio Listener
                                      }
        );
    }

    void ComponentDrawerRegistry::registerAudioEmitter() {
        registerDrawer<AudioEmitter>("Audio Emitter",
                                     [](AudioEmitter& component, Entity entity, Scene& scene) {
                                         ImGui::DragFloat("Volume", &component.volume, 0.01f, 0.0f, 1.0f, "%.2f");
                                         ImGui::DragFloat("Pitch", &component.pitch, 0.01f, 0.0f, 1.0f);
                                         ImGui::Checkbox("Loop", &component.loop);
                                     }
        );
    }
}
