#pragma once

#include "Component.hpp"
#include <glm/vec3.hpp>


namespace TechEngine {
    class Scene;
    class Transform;
}

namespace TechEngineAPI {
    class API_DLL Transform : public Component {
    public:
        TechEngine::Transform* m_transform;

        Transform(Entity entity, TechEngine::Transform* transform): Component(entity), m_transform(transform) {
        }

        void updateInternalPointer(TechEngine::Scene* scene) override;

        void setPosition(const glm::vec3& position);

        void setRotation(const glm::vec3& rotation);

        void setScale(const glm::vec3& scale);

        glm::vec3 getPosition();

        glm::vec3 getRotation();

        glm::vec3 getScale();
    };
}
