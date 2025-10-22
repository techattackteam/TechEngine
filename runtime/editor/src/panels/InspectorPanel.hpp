#pragma once

#include "Panel.hpp"
#include "systems/SystemsRegistry.hpp"
#include "scene/ScenesManager.hpp"
#include "HierarchyNode.hpp"

#include <imgui_internal.h>

#include "renderer/Renderer.hpp"


namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
        SystemsRegistry& m_appSystemsRegistry;
        HierarchyNode& m_selectedNode;

    public:
        InspectorPanel(SystemsRegistry& editorSystemRegistry, SystemsRegistry& appSystemRegistry, HierarchyNode& selectedNode);

        void onInit() override;

        void onUpdate() override;

        void drawComponents();

        void openAddComponentMenu();

        template<typename T, typename UIFunction, typename RemoveFunction = std::function<void()>>
        void drawComponent(Entity entity, const std::string& name, UIFunction uiFunction, RemoveFunction removeFunction = [] {
                           }) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
            if (scene.hasComponent<T>(entity)) {
                auto& component = scene.getComponent<T>(entity);
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
                    scene.removeComponent<T>(entity);
                    removeFunction();
                }
            }
        }

        template<class C, class... A>
        C& addComponent(A&... args) {
            Scene& scene = m_appSystemsRegistry.getSystem<ScenesManager>().getActiveScene();
            Entity entity = m_selectedNode.entity;
            scene.addComponent<C>(entity, C(args...));
            return scene.getComponent<C>(entity);
        }

        void drawWidgetProperties();

        template<typename T>
        void inspectTextWidget(T* widget) {
            std::string text = widget->getText();

            if (resizableInputTextMultiline("##Label", &text, ImVec2(-1.0f, ImGui::GetTextLineHeight() * 4), ImGui::GetTextLineHeight() * 4)) {
                widget->setText(text);
            }

            ImGui::ColorEdit4("Color", &widget->getColor().x);
            // Font Selection (assuming you have a FontManager)
            // For now, let's use a placeholder.
            // To implement this fully, your FontManager should provide a vector of font names.
            const char* fonts[] = {"Arial", "Roboto", "Times New Roman"};
            static int currentFont = 0;
            if (ImGui::Combo("Font", &currentFont, fonts, IM_ARRAYSIZE(fonts))) {
                // Example: setFontPath(fontManager->getPathFor(fonts[currentFont]));
            }

            // Font Size
            if (ImGui::DragFloat("Font Size", &widget->getFontSize(), 0.5f, 8.0f, 72.0f)) {
                // Value is updated directly
            }

            // Font Style
            ImGui::Checkbox("Bold", &widget->isBold());
            ImGui::SameLine();
            ImGui::Checkbox("Italic", &widget->isItalic());

            // Horizontal Alignment
            const char* h_align_items[] = {"Left", "Center", "Right"};
            int current_h_align = static_cast<int>(widget->getHorizontalAlign());
            if (ImGui::Combo("H-Align", &current_h_align, h_align_items, IM_ARRAYSIZE(h_align_items))) {
                //m_hAlign = static_cast<HorizontalAlignment>(current_h_align);
            }

            // Vertical Alignment
            const char* v_align_items[] = {"Top", "Middle", "Bottom"};
            int current_v_align = static_cast<int>(widget->getVerticalAlign());
            if (ImGui::Combo("V-Align", &current_v_align, v_align_items, IM_ARRAYSIZE(v_align_items))) {
                //m_vAlign = static_cast<VerticalAlignment>(current_v_align);
            }
        }

        bool resizableInputTextMultiline(const char* label, std::string* text, const ImVec2& initial_size, float min_height);

        template<typename UIFunction>
        void drawRenderPassProperty(const std::string& passName, UIFunction uiFunction) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();

            //ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            //float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", passName.c_str());
            ImGui::PopStyleVar();
            //ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

            if (open) {
                uiFunction(renderer);
                ImGui::TreePop();
            }
        }

        void drawRenderPassProperties();
    };
}
