#include "components/MeshRenderer.hpp"

#include "components/Components.hpp"
#include "../../../core/src/scene/Scene.hpp"

namespace TechEngineAPI {
    void MeshRenderer::updateInternalPointer(TechEngine::Scene* scene) {
        m_meshRenderer = &scene->getComponent<TechEngine::MeshRenderer>(m_entity);
    }

    void MeshRenderer::paintMesh() {
        m_meshRenderer->paintMesh();
    }
}
