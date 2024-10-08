#include "common/include/components/Tag.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    void Tag::updateInternalPointer(TechEngine::Scene* scene) {
        m_tag = &scene->getComponent<TechEngine::Tag>(m_entity);
    }
}
