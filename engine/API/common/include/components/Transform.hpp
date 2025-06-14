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

        Transform(const Entity entity, TechEngine::Transform* transform): Component(entity), m_transform(transform) {
        }

        void updateInternalPointer(TechEngine::Scene* scene) override;

        void setPosition(const glm::vec3& position) const;

        void setRotation(const glm::vec3& rotation) const;

        void setScale(const glm::vec3& scale) const;

        glm::vec3 getPosition() const;

        glm::vec3 getRotation() const;

        glm::vec3 getScale() const;

        glm::vec3 getUp() const;

        glm::vec3 getForward() const;

        glm::vec3 getRight() const;
    };
}
