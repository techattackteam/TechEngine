#include <ios>
#include <imgui_internal.h>
#include "SceneHierarchyPanel.hpp"
#include "scene/SceneHelper.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"
#include "PanelsManager.hpp"
#include "defaultGameObject/Cube.hpp"
#include "defaultGameObject/Sphere.hpp"
#include "defaultGameObject/Cylinder.hpp"
//#include "defaultGameObject/Cylinder.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel() : Panel("SceneHierarchyPanel") {

    }

    void SceneHierarchyPanel::onUpdate() {
        ImGui::Begin("Scene Hierarchy");
        if (!scene.getGameObjects().empty()) {
            for (auto element: scene.getGameObjects()) {
                if (element->isEditorOnly()) {
                    continue;
                }
                drawEntityNode(element);
            }
            if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
                selectedGO = nullptr;
                PanelsManager::getInstance().deselectGameObject();
            }
            if (!ImGui::IsItemHovered() && ImGui::BeginPopupContextWindow(0, 1)) {
                if (ImGui::BeginMenu("Create")) {
                    if (ImGui::MenuItem("Empty")) {
                        new GameObject("Empty");
                    }
                    if (ImGui::MenuItem("Cube")) {
                        new Cube();
                    }
                    if (ImGui::MenuItem("Sphere")) {
                        new Sphere();
                    }
                    if (ImGui::MenuItem("Cylinder")) {
                        new Cylinder();
                    }
                    ImGui::EndMenu();
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
            PanelsManager::getInstance().selectGameObject(gameObject->getTag());
        }

        if (ImGui::BeginPopupContextItem("GameObject Menu", 1)) {
            if (ImGui::MenuItem("Make Child (WIP)")) {
/*                GameObject *child = new QuadMeshTest(gameObject->getName() + "'s Child");
                Scene::getInstance().makeChildTo(gameObject, child);*/
            }
            if (ImGui::MenuItem("Duplicate (WIP)")) {
                //new QuadMeshTest(gameObject->getName() + "'s duplicate");
            }
            if (ImGui::MenuItem("Delete GameObject")) {
                TechEngine::EventDispatcher::getInstance().dispatch(new RequestDeleteGameObject(gameObject));
                PanelsManager::getInstance().deselectGameObject(gameObject->getTag());
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
