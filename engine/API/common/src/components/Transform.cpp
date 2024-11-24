#include "common/include/components/Transform.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    void Transform::updateInternalPointer(TechEngine::Scene* scene) {
        m_transform = &scene->getComponent<TechEngine::Transform>(m_entity);
    }

    void Transform::setPosition(const glm::vec3& position) const {
        m_transform->position = (position);
        TE_LOGGER_INFO("M_Transform position {0} {1} {2}", m_transform->position.x, m_transform->position.y, m_transform->position.z);
    }

    void Transform::setRotation(const glm::vec3& rotation) const {
        m_transform->rotation = (rotation);
    }

    void Transform::setScale(const glm::vec3& scale) const {
        m_transform->scale = (scale);
    }

    glm::vec3 Transform::getPosition() const {
        return m_transform->position;
    }

    glm::vec3 Transform::getRotation() const {
        return m_transform->rotation;
    }

    glm::vec3 Transform::getScale() const {
        return m_transform->scale;
    }
}
