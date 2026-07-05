#include "InspectorPanel.hpp"

#include "core/Logger.hpp"
#include "scene/CameraSystem.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/components/ComponentsFactory.hpp"

#include "physics/PhysicsEngine.hpp"
#include "renderer/Renderer.hpp"
#include "resources/ResourceSystem.hpp"
#include "scene/SceneInternal.hpp"
#include "scene/SceneManager.hpp"
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
                                                                  m_componentDrawerRegistry(m_appSystemsRegistry),
                                                                  Panel(editorSystemRegistry) {
    }

    void InspectorPanel::onInit() {
        m_componentDrawerRegistry.registerAllDrawers();
    }

    void InspectorPanel::onUpdate() {
        if (m_selectedNode.type == HierarchyNode::NodeType::Entity) {
            Entity entity = m_selectedNode.entity;
            Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
            drawEntityHeader(entity, scene);
            m_componentDrawerRegistry.drawAll(entity, scene,
                                              [this](const ComponentDrawer& drawer, Entity entity, Scene& scene) {
                                                  drawComponentFrame(drawer, entity, scene);
                                              });
            openAddComponentMenu();
        } else if (m_selectedNode.type == HierarchyNode::NodeType::Widget) {
            drawWidgetProperties();
        } else if (m_selectedNode.type == HierarchyNode::NodeType::RenderPass) {
            drawRenderPassProperties();
        }
    }

    void InspectorPanel::drawEntityHeader(Entity entity, Scene& scene) {
        Tag& tag = scene.getComponent<Tag>(entity);
        std::string name = tag.getName();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##EntityName", &name)) {
            tag.setName(name);
        }
        ImGui::PopItemWidth();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
        ImGui::TextUnformatted(tag.getUuid().c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
    }

    void InspectorPanel::drawComponentFrame(const ComponentDrawer& drawer, Entity entity, Scene& scene) {
        constexpr ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_DefaultOpen |
                ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowItemOverlap |
                ImGuiTreeNodeFlags_FramePadding;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4, 4});
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
        const float lineH = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
        ImGui::Separator();

        const ImGuiID nodeId = ImGui::GetID(drawer.displayName.c_str());
        const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(nodeId)),
                                            flags, "%s", drawer.displayName.c_str());

        ImGui::PopStyleVar();

        ImGui::SameLine(contentRegionAvailable.x - lineH * 0.5f);

        const std::string popupId = "##settings_" + drawer.displayName;
        if (ImGui::Button(("-##" + drawer.displayName).c_str(), {lineH, lineH})) {
            ImGui::OpenPopup(popupId.c_str());
        }

        bool remove = false;
        if (ImGui::BeginPopup(popupId.c_str())) {
            if (ImGui::MenuItem("Remove component")) {
                remove = true;
            }
            ImGui::EndPopup();
        }

        if (open) {
            drawer.draw(entity, scene);
            ImGui::TreePop();
        }

        if (remove) {
            drawer.onRemove(entity, scene);
        }
    }

    void InspectorPanel::openAddComponentMenu() {
        if (ImGui::BeginPopupContextWindow("Add Component", 1)) {
            Entity entity = m_selectedNode.entity;
            if (ImGui::MenuItem("Camera")) {
                addComponent<Camera>();
            }
            if (ImGui::MenuItem("Mesh Renderer")) {
                const std::shared_ptr<MeshResource> mesh = m_appSystemsRegistry.getSystem<ResourceSystem>().getDefaultMesh();
                const std::shared_ptr<MaterialResource> material = m_appSystemsRegistry.getSystem<ResourceSystem>().getDefaultMaterial();
                MeshRenderer& meshRenderer = addComponent<MeshRenderer>();
                meshRenderer.changeMaterial(material->getUUID());
                meshRenderer.changeMesh(mesh->getUUID());
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Point Light")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                    scene.addComponent<PointLight>(entity, ComponentsFactory::createPointLight());
                } else if (ImGui::MenuItem("Directional Light")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                    scene.addComponent<DirectionalLight>(entity, ComponentsFactory::createDirectionalLight());
                } else if (ImGui::MenuItem("SpotLight")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                    scene.addComponent<SpotLight>(entity, ComponentsFactory::createSpotLight());
                }

                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Physics")) {
                if (ImGui::BeginMenu("Bodies")) {
                    if (ImGui::MenuItem("Static Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<StaticBody>(entity, ComponentsFactory::createStaticBody(physicsEngine, tag, transform));
                    }
                    if (ImGui::MenuItem("Kinematic Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<KinematicBody>(entity, ComponentsFactory::createKinematicBody(physicsEngine, tag, transform));
                    }
                    if (ImGui::MenuItem("Rigid Body")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<RigidBody>(entity, ComponentsFactory::createRigidBody(physicsEngine, tag, transform));
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Colliders")) {
                    if (ImGui::MenuItem("Box Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<BoxCollider>(entity, ComponentsFactory::createBoxCollider(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                    }
                    if (ImGui::MenuItem("Sphere Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<SphereCollider>(entity, ComponentsFactory::createSphereCollider(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                    }
                    if (ImGui::MenuItem("Capsule Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CapsuleCollider>(entity, ComponentsFactory::createCapsuleCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    if (ImGui::MenuItem("Cylinder Collider")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CylinderCollider>(entity, ComponentsFactory::createCylinderCollider(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Triggers")) {
                    if (ImGui::MenuItem("Box Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<BoxTrigger>(entity, ComponentsFactory::createBoxTrigger(physicsEngine, tag, transform, glm::vec3(0, 0, 0), glm::vec3(1)));
                    }

                    if (ImGui::MenuItem("Sphere Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<SphereTrigger>(entity, ComponentsFactory::createSphereTrigger(physicsEngine, tag, transform, glm::vec3(0), 0.5f));
                    }

                    if (ImGui::MenuItem("Capsule Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                        Tag& tag = scene.getComponent<Tag>(entity);
                        Transform& transform = scene.getComponent<Transform>(entity);
                        PhysicsEngine& physicsEngine = m_appSystemsRegistry.getSystem<PhysicsEngine>();
                        scene.addComponent<CapsuleTrigger>(entity, ComponentsFactory::createCapsuleTrigger(physicsEngine, tag, transform, glm::vec3(0), 1.0f, 0.5f));
                    }
                    if (ImGui::MenuItem("Cylinder Trigger")) {
                        Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
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
                    Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
                    scene.addComponent<AudioListener>(entity, ComponentsFactory::createAudioListener());
                }
                if (ImGui::MenuItem("Emitter")) {
                    Scene& scene = m_appSystemsRegistry.getSystem<SceneManager>().getActiveScene();
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
            } else if ("LightCulling") {
                ImGui::Checkbox("Enabled", &renderer.getDebugLightCullingEnabled());
            }
        });
    }
}
