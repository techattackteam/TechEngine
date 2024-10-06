#include "components/Transform.hpp"

#include "../../../core/src/scene/Scene.hpp"

namespace TechEngineAPI {
    void Transform::updateInternalPointer(TechEngine::Scene* scene) {
        m_transform = &scene->getComponent<TechEngine::Transform>(m_entity);
    }

    void Transform::setPosition(const glm::vec3& position) {
        m_transform->position = (position);
    }

    void Transform::setRotation(const glm::vec3& rotation) {
        m_transform->rotation = (rotation);
    }

    void Transform::setScale(const glm::vec3& scale) {
        m_transform->scale = (scale);
    }

    glm::vec3 Transform::getPosition() {
        return m_transform->position;
    }

    glm::vec3 Transform::getRotation() {
        return m_transform->rotation;
    }

    glm::vec3 Transform::getScale() {
        return m_transform->scale;
    }
}
