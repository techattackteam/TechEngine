#include "SceneHierarchyPanel.hpp"
#include "PanelsManager.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel(const std::string& name,
                                             SystemsRegistry& editorRegistry,
                                             SystemsRegistry& appRegistry) : appRegistry(appRegistry), Panel(name, editorRegistry) {
    }

    void SceneHierarchyPanel::init() {
        Panel::init();
    }

    void SceneHierarchyPanel::onUpdate() {
        isItemHovered = false;
        getSelectedGO();
        if (!appRegistry.getSystem<SceneManager>().getScene().getGameObjects().empty()) {
            int originalSize = appRegistry.getSystem<SceneManager>().getScene().getGameObjects().size();
            for (int i = 0; i < appRegistry.getSystem<SceneManager>().getScene().getGameObjects().size(); i++) {
                GameObject* element = appRegistry.getSystem<SceneManager>().getScene().getGameObjects()[i];
                if (element->isEditorOnly() || element->getParent() != nullptr) {
                    continue;
                }
                if (originalSize != appRegistry.getSystem<SceneManager>().getScene().getGameObjects().size()) {
                    i--;
                    originalSize = appRegistry.getSystem<SceneManager>().getScene().getGameObjects().size();
                }
                drawEntityNode(element);
            }

            if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
                selectedGO.clear();
            }

            if (!isItemHovered && ImGui::BeginPopupContextWindow()) {
                if (ImGui::BeginMenu("Create")) {
                    if (ImGui::MenuItem("Empty")) {
                        appRegistry.getSystem<SceneManager>().getScene().createGameObject("Empty");
                    }
                    if (ImGui::MenuItem("Cube")) {
                        GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Cube");
                        gameObject.addComponent<MeshRendererComponent>();
                    }
                    if (ImGui::MenuItem("Sphere")) {
                        GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Sphere");
                        gameObject.addComponent<MeshRendererComponent>();
                    }
                    if (ImGui::MenuItem("Cylinder")) {
                        GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Cylinder");
                        gameObject.addComponent<MeshRendererComponent>();
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
                    for (GameObject* go: appRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
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
                GameObject& child = appRegistry.getSystem<SceneManager>().getScene().createGameObject(gameObject->getName() + "'s Child");
                appRegistry.getSystem<SceneManager>().getScene().makeChildTo(gameObject, &child);
            }
            if (ImGui::MenuItem("Duplicate")) {
                appRegistry.getSystem<SceneManager>().getScene().duplicateGameObject(gameObject);
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
                    appRegistry.getSystem<SceneManager>().getScene().duplicateGameObject(gameObject);
                }
            } else if (key.getKeyCode() == KeyCode::F) {
                if (!selectedGO.empty()) {
                    //sceneView.focusOnGameObject(selectedGO.front());
                }
            } else if (key.getKeyCode() == KeyCode::W) {
                if (!selectedGO.empty()) {
                    GameObject& child = appRegistry.getSystem<SceneManager>().getScene().createGameObject(selectedGO.front()->getName() + "'s Child");
                    appRegistry.getSystem<SceneManager>().getScene().makeChildTo(selectedGO.front(), &child);
                }
            }
        }
    }

    void SceneHierarchyPanel::deleteGameObject(GameObject* gameObject) {
        if (std::find(selectedGO.begin(), selectedGO.end(), gameObject) != selectedGO.end()) {
            selectedGO.erase(std::remove(selectedGO.begin(), selectedGO.end(), gameObject), selectedGO.end());
        }
        appRegistry.getSystem<SceneManager>().getScene().deleteGameObject(gameObject);
    }

    std::vector<GameObject*>& SceneHierarchyPanel::getSelectedGO() {
        for (int i = 0; i < selectedGO.size(); i++) {
            GameObject* gameObject = appRegistry.getSystem<SceneManager>().getScene().getGameObjectByTag(selectedGO.front()->getTag());
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
