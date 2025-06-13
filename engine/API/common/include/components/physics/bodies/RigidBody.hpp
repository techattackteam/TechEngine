#pragma once

#include <common/include/components/Component.hpp>

#include "common/include/scene/Scene.hpp"
#include "glm/vec3.hpp"

namespace TechEngine {
    class Scene;
    class RigidBody;
}

namespace TechEngineAPI {
    class API_DLL RigidBody : public Component {
    private:
        TechEngine::RigidBody* m_rigidBody = nullptr;
        TechEngine::Scene* m_scene = nullptr;
        TechEngine::PhysicsEngine* m_physicsEngine = nullptr;

    public:
        RigidBody(Entity entity, TechEngine::PhysicsEngine* physicsEngine, TechEngine::Scene* scene);

        void updateInternalPointer(TechEngine::Scene* scene) override;

        void setVelocity(const glm::vec3& velocity) const;

        void addVelocity(const glm::vec3& velocity) const;
    };
}
