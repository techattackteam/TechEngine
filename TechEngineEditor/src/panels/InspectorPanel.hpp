#pragma once

#include "CustomPanel.hpp"
#include "scene/GameObject.hpp"
#include "material/MaterialManager.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
        std::vector<GameObject*>& selectedGameObjects;
        bool m_isActive = false;

        MaterialManager& materialManager;
        PhysicsEngine& physicsEngine;

    public:
        explicit InspectorPanel(const std::string& name, EventDispatcher& eventDispatcher, std::vector<GameObject*>& selectedGameObjects, MaterialManager& materialManager, PhysicsEngine& physicsEngine);

        void onUpdate() override;

        template<typename T, typename UIFunction>
        void drawComponent(GameObject* gameObject, const std::string& name, UIFunction uiFunction);

        void drawComponents();

    private:
        template<typename C, typename... A>
        void addComponent(A&... args);

        void drawCommonComponents();
    };
}
