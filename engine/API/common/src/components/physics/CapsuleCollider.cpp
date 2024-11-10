#include "common/include/components/physics/CapsuleCollider.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    CapsuleCollider::CapsuleCollider(const Entity entity) : Component(entity) {
    }

    void CapsuleCollider::setCenter(const glm::vec3 center) const {
        m_capsuleCollider->center = center;
    }

    void CapsuleCollider::setHeight(const float height) const {
        m_capsuleCollider->height = height;
    }

    void CapsuleCollider::setRadius(const float radius) const {
        m_capsuleCollider->radius = radius;
    }

    glm::vec3 CapsuleCollider::getCenter() const {
        return m_capsuleCollider->center;
    }

    float CapsuleCollider::getHeight() const {
        return m_capsuleCollider->height;
    }

    float CapsuleCollider::getRadius() const {
        return m_capsuleCollider->radius;
    }

    void CapsuleCollider::updateInternalPointer(TechEngine::Scene* scene) {
        m_capsuleCollider = &scene->getComponent<TechEngine::CapsuleCollider>(m_entity);
    }
}
