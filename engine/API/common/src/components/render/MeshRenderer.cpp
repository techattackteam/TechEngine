#include "common/include/components/render/MeshRenderer.hpp"

#include "components/Components.hpp"
#include "scene/Scene.hpp"

namespace TechEngineAPI {
    void MeshRenderer::updateInternalPointer(TechEngine::Scene* scene) {
        m_meshRenderer = &scene->getComponent<TechEngine::MeshRenderer>(m_entity);
    }
}
