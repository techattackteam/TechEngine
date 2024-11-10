#include "common/include/components/physics/SphereCollider.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    SphereCollider::SphereCollider(const Entity entity) : Component(entity) {
    }

    void SphereCollider::setCenter(const glm::vec3 center) const {
        m_sphereCollider->center = center;
    }

    void SphereCollider::setRadius(const float radius) const {
        m_sphereCollider->radius = radius;
    }

    glm::vec3 SphereCollider::getCenter() const {
        return m_sphereCollider->center;
    }


    float SphereCollider::getRadius() const {
        return m_sphereCollider->radius;
    }

    void SphereCollider::updateInternalPointer(TechEngine::Scene* scene) {
        m_sphereCollider = &scene->getComponent<TechEngine::SphereCollider>(m_entity);
    }
}
