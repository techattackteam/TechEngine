#include "common/include/components/physics/triggers/CapsuleTrigger.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    CapsuleTrigger::CapsuleTrigger(const Entity entity) : Component(entity) {
    }

    void CapsuleTrigger::setCenter(const glm::vec3 center) const {
        m_capsuleTrigger->center = center;
    }

    void CapsuleTrigger::setHeight(const float height) const {
        m_capsuleTrigger->height = height;
    }

    void CapsuleTrigger::setRadius(const float radius) const {
        m_capsuleTrigger->radius = radius;
    }

    glm::vec3 CapsuleTrigger::getCenter() const {
        return m_capsuleTrigger->center;
    }

    float CapsuleTrigger::getHeight() const {
        return m_capsuleTrigger->height;
    }

    float CapsuleTrigger::getRadius() const {
        return m_capsuleTrigger->radius;
    }

    void CapsuleTrigger::updateInternalPointer(TechEngine::Scene* scene) {
        m_capsuleTrigger = &scene->getComponent<TechEngine::CapsuleCollider>(m_entity);
    }
}
