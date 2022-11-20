#include <ios>
#include <imgui_internal.h>
#include "SceneHierarchyPanel.hpp"
#include "scene/Scene.hpp"
#include "events/OnSelectGameObjectEvent.hpp"
#include "testGameObject/QuadMeshTest.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel() : Panel("SceneHierarchyPanel") {

    }

    void SceneHierarchyPanel::onUpdate() {
        Scene &scene = Scene::getInstance();
        ImGui::Begin("Scene Hierarchy");
        if (!scene.getGameObjects().empty()) {
            for (auto element: Scene::getInstance().getGameObjects()) {
                drawEntityNode(element);
            }

            if (ImGui::BeginPopupContextWindow(0, 1, false)) {
                if (ImGui::MenuItem("New Game Object")) {
                    new QuadMeshTest("QuadMeshTest");
                }

                ImGui::EndPopup();
            }

        }
        ImGui::End();
    }

    void SceneHierarchyPanel::drawEntityNode(GameObject *gameObject) {
        std::string name = gameObject->getName();

        ImGuiTreeNodeFlags flags = ((selectedGO == gameObject) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow |
                                   (gameObject->getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0);
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void *) (uint64_t) (uint32_t) gameObject, flags, name.c_str());
        if (ImGui::IsItemClicked()) {
            selectedGO = gameObject;
            TechEngineCore::EventDispatcher::getInstance().dispatch(new OnSelectGameObjectEvent(gameObject));
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Make Child")) {
                GameObject *child = new QuadMeshTest(gameObject->getName() + "'s Child");
                CoreScene::getInstance().makeChildTo(gameObject, child);
            }

            if (ImGui::MenuItem("Delete GameObject")) {
                TechEngineCore::EventDispatcher::getInstance().dispatch(new GameObjectDestroyEvent(gameObject));
            }

            ImGui::EndPopup();
        }

        if (opened) {
            for (const auto& pair: gameObject->getChildren()) {
                drawEntityNode(pair.second);
            }
            ImGui::TreePop();
        }

    }
}
