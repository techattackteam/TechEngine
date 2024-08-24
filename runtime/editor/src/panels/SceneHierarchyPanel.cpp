#include "SceneHierarchyPanel.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel(SystemsRegistry& systemRegistry,
                                             SystemsRegistry& appSystemRegistry,
                                             std::vector<Entity>& selectedEntities) : m_systemRegistry(systemRegistry),
                                                                                      m_appSystemRegistry(appSystemRegistry),
                                                                                      m_selectedEntities(selectedEntities) {
    }


    void SceneHierarchyPanel::onInit() {
        Scene& scene = m_appSystemRegistry.getSystem<Scene>();
        int entity;
        /*entity = scene.createEntity();
        scene.addComponent<Transform>(entity);*/
    }

    void SceneHierarchyPanel::onUpdate() {
        isItemHovered = false;
        Scene& scene = m_appSystemRegistry.getSystem<Scene>();
        if (!scene.getEntities().empty()) {
            int originalSize = scene.getEntities().size();
            for (int i = 0; i < scene.getEntities().size(); i++) {
                Entity entity = scene.getEntities()[i];
                /*if (element->isEditorOnly() || element->getParent() != nullptr) {
                    continue;
                }*/
                if (originalSize != scene.getEntities().size()) {
                    i--;
                    originalSize = scene.getEntities().size();
                }
                drawEntityNode(entity);
            }
        }
        if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
            m_selectedEntities.clear();
        }

        if (!isItemHovered && ImGui::BeginPopupContextWindow()) {
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Empty")) {
                    scene.createEntity("New Entity");
                }
                if (ImGui::MenuItem("Cube")) {
                    /*GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Cube");
                    gameObject.addComponent<MeshRendererComponent>();*/
                }
                if (ImGui::MenuItem("Sphere")) {
                    /*GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Sphere");
                    gameObject.addComponent<MeshRendererComponent>();*/
                }
                if (ImGui::MenuItem("Cylinder")) {
                    /*GameObject& gameObject = appRegistry.getSystem<SceneManager>().getScene().createGameObject("Cylinder");
                    gameObject.addComponent<MeshRendererComponent>();*/
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
    }

    void SceneHierarchyPanel::drawEntityNode(Entity entity) {
        Scene& scene = m_appSystemRegistry.getSystem<Scene>();
        Tag& tag = scene.getComponent<Tag>(entity);
        ImGuiTreeNodeFlags flags = ((std::find(m_selectedEntities.begin(), m_selectedEntities.end(), entity) != m_selectedEntities.end()) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   /*(entity->getChildren().empty() ? ImGuiTreeNodeFlags_Leaf : 0)*/ ImGuiTreeNodeFlags_Leaf |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((tag.name + tag.uuid).c_str(), flags, "%s", tag.name.c_str());
        if (ImGui::IsItemClicked()) {
            /*if (isCtrlPressed) {
                if (std::find(m_selectedEntities.begin(), m_selectedEntities.end(), gameObject) != m_selectedEntities.end()) {
                    m_selectedEntities.erase(std::remove(m_selectedEntities.begin(), m_selectedEntities.end(), gameObject), m_selectedEntities.end());
                } else {
                    m_selectedEntities.push_back(gameObject);
                }
            } else if (isShiftPressed) {
                if (m_selectedEntities.empty()) {
                    m_selectedEntities.push_back(gameObject);
                } else {
                    GameObject* first = m_selectedEntities.front();

                    bool selecting = false;
                    for (GameObject* go: appRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
                        if (go->isEditorOnly()) {
                            continue;
                        }
                        if (go == first && !selecting || go == gameObject && !selecting) {
                            selecting = true;
                            m_selectedEntities.push_back(go);
                        } else if (go == gameObject && selecting || go == first && selecting) {
                            selecting = false;
                            m_selectedEntities.push_back(go);
                        } else if (selecting) {
                            m_selectedEntities.push_back(go);
                        }
                    }
                }
            } else {
            }*/
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);
        }
        if (ImGui::IsItemHovered()) {
            isItemHovered = true;
        }
        if (ImGui::BeginPopupContextItem()) {
            m_selectedEntities.clear();
            m_selectedEntities.push_back(entity);
            std::string name = tag.name;
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "Name", name)) {
                //gameObject->setName(name);
            }
            if (ImGui::MenuItem("Make Child (WIP)")) {
                /*GameObject& child = appRegistry.getSystem<SceneManager>().getScene().createGameObject(gameObject->getName() + "'s Child");
                appRegistry.getSystem<SceneManager>().getScene().makeChildTo(gameObject, &child);*/
            }
            if (ImGui::MenuItem("Duplicate")) {
                /*appRegistry.getSystem<SceneManager>().getScene().duplicateGameObject(gameObject);*/
            }
            if (ImGui::MenuItem("Delete GameObject")) {
                scene.destroyEntity(entity);
            }
            ImGui::EndPopup();
        }
        if (opened) {
            //Draw Entity Child Nodes
            /*if (gameObject != nullptr) {
                for (const auto& pair: gameObject->getChildren()) {
                    drawEntityNode(pair.second);
                }
            }*/
            ImGui::TreePop();
        }
    }
}
