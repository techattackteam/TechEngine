#include "common/include/components/physics/colliders/CylinderCollider.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    CylinderCollider::CylinderCollider(const Entity entity) : Component(entity) {
    }

    void CylinderCollider::setCenter(const glm::vec3 center) const {
        m_cylinderCollider->center = center;
    }

    void CylinderCollider::setHeight(float height) const {
        m_cylinderCollider->height = height;
    }

    void CylinderCollider::setRadius(const float radius) const {
        m_cylinderCollider->radius = radius;
    }

    glm::vec3 CylinderCollider::getCenter() const {
        return m_cylinderCollider->center;
    }

    float CylinderCollider::getHeight() const {
        return m_cylinderCollider->height;
    }


    float CylinderCollider::getRadius() const {
        return m_cylinderCollider->radius;
    }

    void CylinderCollider::updateInternalPointer(TechEngine::Scene* scene) {
        m_cylinderCollider = &scene->getComponent<TechEngine::CylinderCollider>(m_entity);
    }
}
