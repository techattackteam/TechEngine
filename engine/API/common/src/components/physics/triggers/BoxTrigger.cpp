#include "common/include/components/physics/triggers/BoxTrigger.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"


namespace TechEngineAPI {
    void BoxTrigger::setCenter(const glm::vec3 center) const {
        m_boxTrigger->center = center;
    }

    void BoxTrigger::setSize(const glm::vec3 size) const {
        m_boxTrigger->size = size;
    }

    glm::vec3 BoxTrigger::getCenter() const {
        return m_boxTrigger->center;
    }

    glm::vec3 BoxTrigger::getSize() const {
        return m_boxTrigger->size;
    }

    void BoxTrigger::updateInternalPointer(TechEngine::Scene* scene) {
        m_boxTrigger = &scene->getComponent<TechEngine::BoxTrigger>(m_entity);
    }
}
