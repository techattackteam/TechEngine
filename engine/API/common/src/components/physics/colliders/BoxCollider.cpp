#include "common/include/components/physics/colliders/BoxCollider.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"


namespace TechEngineAPI {
    void BoxCollider::setCenter(const glm::vec3 center) const {
        m_boxCollider->center = center;
    }

    void BoxCollider::setSize(const glm::vec3 size) const {
        m_boxCollider->size = size;
    }

    glm::vec3 BoxCollider::getCenter() const {
        return m_boxCollider->center;
    }

    glm::vec3 BoxCollider::getSize() const {
        return m_boxCollider->size;
    }

    void BoxCollider::updateInternalPointer(TechEngine::Scene* scene) {
        m_boxCollider = &scene->getComponent<TechEngine::BoxCollider>(m_entity);
    }
}
