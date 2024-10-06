#include "components/Tag.hpp"

#include "components/Components.hpp"
#include "../../../core/src/scene/Scene.hpp"

namespace TechEngineAPI {
    void Tag::updateInternalPointer(TechEngine::Scene* scene) {
        m_tag = &scene->getComponent<TechEngine::Tag>(m_entity);
    }
}
