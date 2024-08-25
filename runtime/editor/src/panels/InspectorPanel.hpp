#pragma once

#include <imgui_internal.h>

#include "Panel.hpp"
#include "scene/Scene.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_appSystemRegistry;
        const std::vector<Entity>& m_selectedEntities;

    public:
        InspectorPanel(SystemsRegistry& systemRegistry, SystemsRegistry& appSystemRegistry, const std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void drawCommonComponents();

        template<typename T, typename UIFunction>
        void drawComponent(Entity entity, const std::string& name, UIFunction uiFunction) {
            const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            Scene& scene = m_appSystemRegistry.getSystem<Scene>();
            if (scene.hasComponent<T>(entity)) {
                auto component = scene.getComponent<T>(entity);
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
                }
            }
        }

        template<class C, class... A>
        void addComponent(A&... args) {
            Scene& scene = m_appSystemRegistry.getSystem<Scene>();
            for (const int& entity: m_selectedEntities) {
                scene.addComponent<C>(entity, args...);
            }
        }
    };
}
