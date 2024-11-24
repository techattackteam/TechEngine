#include "common/include/components/physics/triggers/CylinderTrigger.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    CylinderTrigger::CylinderTrigger(const Entity entity) : Component(entity) {
    }

    void CylinderTrigger::setCenter(const glm::vec3 center) const {
        m_cylinderTrigger->center = center;
    }

    void CylinderTrigger::setHeight(float height) const {
        m_cylinderTrigger->height = height;
    }

    void CylinderTrigger::setRadius(const float radius) const {
        m_cylinderTrigger->radius = radius;
    }

    glm::vec3 CylinderTrigger::getCenter() const {
        return m_cylinderTrigger->center;
    }

    float CylinderTrigger::getHeight() const {
        return m_cylinderTrigger->height;
    }


    float CylinderTrigger::getRadius() const {
        return m_cylinderTrigger->radius;
    }

    void CylinderTrigger::updateInternalPointer(TechEngine::Scene* scene) {
        m_cylinderTrigger = &scene->getComponent<TechEngine::CylinderTrigger>(m_entity);
    }
}
