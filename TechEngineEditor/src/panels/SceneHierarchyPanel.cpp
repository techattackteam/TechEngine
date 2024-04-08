#include "SceneHierarchyPanel.hpp"
#include "scene/SceneHelper.hpp"
#include "PanelsManager.hpp"
#include "defaultGameObject/Cube.hpp"
#include "defaultGameObject/Sphere.hpp"
#include "defaultGameObject/Cylinder.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel(Scene& scene, MaterialManager& materialManager, SceneView& sceneView) : scene(scene), materialManager(materialManager), sceneView(sceneView), Panel("Scene Hierarchy") {
    }

    void SceneHierarchyPanel::onUpdate() {
        isItemHovered = false;
        getSelectedGO();
        if (!scene.getGameObjects().empty()) {
            int originalSize = scene.getGameObjects().size();
            for (int i = 0; i < scene.getGameObjects().size(); i++) {
                GameObject* element = scene.getGameObjects()[i];
                if (element->isEditorOnly() || element->getParent() != nullptr) {
                    continue;
                }
                if (originalSize != scene.getGameObjects().size()) {
                    i--;
                    originalSize = scene.getGameObjects().size();
                }
                drawEntityNode(element);
            }

            if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
                selectedGO.clear();
            }

            if (!isItemHovered && ImGui::BeginPopupContextWindow()) {
                if (ImGui::BeginMenu("Create")) {
                    if (ImGui::MenuItem("Empty")) {
                        scene.createGameObject<GameObject>("Empty");
                    }
                    if (ImGui::MenuItem("Cube")) {
                        scene.createGameObject<Cube>(&materialManager.getMaterial("DefaultMaterial"));
                    }
                    if (ImGui::MenuItem("Sphere")) {
                        scene.createGameObject<Sphere>(&materialManager.getMaterial("DefaultMaterial"));
                    }
                    if (ImGui::MenuItem("Cylinder")) {
                        scene.createGameObject<Cylinder>(&materialManager.getMaterial("DefaultMaterial"));
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
        }
    }

    void SceneHierarchyPanel::drawEntityNode(GameObject* gameObject) {
        std::string name = gameObject->getName();

        ImGuiTreeNodeFlags flags = ((std::find(selectedGO.begin(), selectedGO.end(), gameObject) != selectedGO.end()) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   (gameObject->getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx(gameObject, flags, "%s", name.c_str());
        if (ImGui::IsItemClicked()) {
            if (isCtrlPressed) {
                if (std::find(selectedGO.begin(), selectedGO.end(), gameObject) != selectedGO.end()) {
                    selectedGO.erase(std::remove(selectedGO.begin(), selectedGO.end(), gameObject), selectedGO.end());
                } else {
                    selectedGO.push_back(gameObject);
                }
            } else if (isShiftPressed) {
                if (selectedGO.empty()) {
                    selectedGO.push_back(gameObject);
                } else {
                    GameObject* first = selectedGO.front();

                    bool selecting = false;
                    for (GameObject* go: scene.getGameObjects()) {
                        if (go->isEditorOnly()) {
                            continue;
                        }
                        if (go == first && !selecting || go == gameObject && !selecting) {
                            selecting = true;
                            selectedGO.push_back(go);
                        } else if (go == gameObject && selecting || go == first && selecting) {
                            selecting = false;
                            selectedGO.push_back(go);
                        } else if (selecting) {
                            selectedGO.push_back(go);
                        }
                    }
                }
            } else {
                selectedGO.clear();
                selectedGO.push_back(gameObject);
            }
        }
        if (ImGui::IsItemHovered()) {
            isItemHovered = true;
        }
        if (ImGui::BeginPopupContextItem()) {
            selectedGO.clear();
            selectedGO.push_back(gameObject);
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                gameObject->setName(name);
            }
            if (ImGui::MenuItem("Make Child (WIP)")) {
                GameObject& child = scene.createGameObject<GameObject>(gameObject->getName() + "'s Child");
                scene.makeChildTo(gameObject, &child);
            }
            if (ImGui::MenuItem("Duplicate")) {
                scene.duplicateGameObject(gameObject);
            }
            if (ImGui::MenuItem("Delete GameObject")) {
                deleteGameObject(gameObject);
                gameObject = nullptr;
            }
            ImGui::EndPopup();
        }
        if (opened) {
            if (gameObject != nullptr) {
                for (const auto& pair: gameObject->getChildren()) {
                    drawEntityNode(pair.second);
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::onKeyPressedEvent(Key& key) {
        if (key.getKeyCode() == KeyCode::LEFT_SHIFT || key.getKeyCode() == KeyCode::RIGHT_SHIFT) {
            isShiftPressed = true;
        } else if (key.getKeyCode() == KeyCode::LEFT_CTRL || key.getKeyCode() == KeyCode::RIGHT_CTRL) {
            isCtrlPressed = true;
        }
        Panel::onKeyPressedEvent(key);
    }

    void SceneHierarchyPanel::onKeyReleasedEvent(TechEngine::Key& key) {
        if (key.getKeyCode() == KeyCode::LEFT_SHIFT || key.getKeyCode() == KeyCode::RIGHT_SHIFT) {
            isShiftPressed = false;
        } else if (key.getKeyCode() == KeyCode::LEFT_CTRL || key.getKeyCode() == KeyCode::RIGHT_CTRL) {
            isCtrlPressed = false;
        }
        Panel::onKeyReleasedEvent(key);
    }

    void SceneHierarchyPanel::processShortcuts() {
        for (Key& key: keysPressed) {
            if (key.getKeyCode() == KeyCode::DEL) {
                for (GameObject* gameObject: selectedGO) {
                    deleteGameObject(gameObject);
                }
                selectedGO.clear();
            } else if (isCtrlPressed && key.getKeyCode() == KeyCode::D) {
                for (GameObject* gameObject: selectedGO) {
                    scene.duplicateGameObject(gameObject);
                }
            } else if (key.getKeyCode() == KeyCode::F) {
                if (!selectedGO.empty()) {
                    sceneView.focusOnGameObject(selectedGO.front());
                }
            } else if (key.getKeyCode() == KeyCode::W) {
                if (!selectedGO.empty()) {
                    GameObject& child = scene.createGameObject<GameObject>(selectedGO.front()->getName() + "'s Child");
                    scene.makeChildTo(selectedGO.front(), &child);
                }
            }
        }
    }

    void SceneHierarchyPanel::deleteGameObject(GameObject* gameObject) {
        if (std::find(selectedGO.begin(), selectedGO.end(), gameObject) != selectedGO.end()) {
            selectedGO.erase(std::remove(selectedGO.begin(), selectedGO.end(), gameObject), selectedGO.end());
        }
        scene.deleteGameObject(gameObject);
    }

    std::vector<GameObject*>& SceneHierarchyPanel::getSelectedGO() {
        for (int i = 0; i < selectedGO.size(); i++) {
            GameObject* gameObject = scene.getGameObjectByTag(selectedGO.front()->getTag());
            if (gameObject == nullptr) {
                selectedGO.erase(selectedGO.begin() + i);
                i--;
            }
        }
        return selectedGO;
    }

    void SceneHierarchyPanel::selectGO(GameObject* selectedGO) {
        this->selectedGO.push_back(selectedGO);
    }

    void SceneHierarchyPanel::deselectGO(GameObject* selectedGO) {
        if (std::find(this->selectedGO.begin(), this->selectedGO.end(), selectedGO) != this->selectedGO.end()) {
            this->selectedGO.erase(std::remove(this->selectedGO.begin(), this->selectedGO.end(), selectedGO), this->selectedGO.end());
        }
    }
}
