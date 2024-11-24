#include "common/include/components/physics/triggers/SphereTrigger.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    SphereTrigger::SphereTrigger(const Entity entity) : Component(entity) {
    }

    void SphereTrigger::setCenter(const glm::vec3 center) const {
        m_sphereTrigger->center = center;
    }

    void SphereTrigger::setRadius(const float radius) const {
        m_sphereTrigger->radius = radius;
    }

    glm::vec3 SphereTrigger::getCenter() const {
        return m_sphereTrigger->center;
    }


    float SphereTrigger::getRadius() const {
        return m_sphereTrigger->radius;
    }

    void SphereTrigger::updateInternalPointer(TechEngine::Scene* scene) {
        m_sphereTrigger = &scene->getComponent<TechEngine::SphereTrigger>(m_entity);
    }
}
