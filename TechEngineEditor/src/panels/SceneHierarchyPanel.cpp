#include <ios>
#include <imgui_internal.h>
#include "SceneHierarchyPanel.hpp"
#include "scene/SceneHelper.hpp"
#include "testGameObject/QuadMeshTest.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"
#include "PanelsManager.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel() : Panel("SceneHierarchyPanel") {

    }

    void SceneHierarchyPanel::onUpdate() {
        ImGui::Begin("Scene Hierarchy");
        if (!scene.getGameObjects().empty()) {
            for (auto element: scene.getGameObjects()) {
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
                                   (gameObject->getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx(gameObject, flags, "%s", name.c_str());
        if (ImGui::IsItemClicked()) {
            selectedGO = gameObject;
            PanelsManager::getInstance().selectedGameObject(gameObject);
        }


        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Make Child")) {
                GameObject *child = new QuadMeshTest(gameObject->getName() + "'s Child");
                Scene::getInstance().makeChildTo(gameObject, child);
            }
            if (ImGui::MenuItem("Duplicate")) {
                new QuadMeshTest(gameObject->getName() + "'s duplicate", gameObject);
            }
            if (ImGui::MenuItem("Delete GameObject")) {
                TechEngine::EventDispatcher::getInstance().dispatch(new RequestDeleteGameObject(gameObject));
                PanelsManager::getInstance().deselectGameObject(gameObject);
                selectedGO = nullptr;
            }

            ImGui::EndPopup();
        }
        if (opened) {
            for (const auto &pair: gameObject->getChildren()) {
                drawEntityNode(pair.second);
            }
            ImGui::TreePop();
        }
    }
}
