#include "common/include/components/Transform.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    void Transform::updateInternalPointer(TechEngine::Scene* scene) {
        m_transform = &scene->getComponent<TechEngine::Transform>(m_entity);
    }

    void Transform::setPosition(const glm::vec3& position) const {
        m_transform->translateTo(position);
    }

    void Transform::setRotation(const glm::vec3& rotation) const {
        m_transform->setRotation(rotation);
    }

    void Transform::setScale(const glm::vec3& scale) const {
        m_transform->setScale(scale);
    }

    glm::vec3 Transform::getPosition() const {
        return m_transform->m_position;
    }

    glm::vec3 Transform::getRotation() const {
        return m_transform->m_rotation;
    }

    glm::vec3 Transform::getScale() const {
        return m_transform->m_scale;
    }

    glm::vec3 Transform::getUp() const {
        return m_transform->m_up;
    }

    glm::vec3 Transform::getForward() const {
        return m_transform->m_forward;
    }

    glm::vec3 Transform::getRight() const {
        return m_transform->m_right;
    }
}
