#include <scene/SceneAssetIdCollector.hpp>

namespace TechEngine {
    const std::vector<ResourceId>& SceneAssetIdCollector::getIds() const {
        return m_ids;
    }

    void SceneAssetIdCollector::visitEntity(Entity&) {
    }

    void SceneAssetIdCollector::visitAsset(ResourceId& id, ResourceSlotId&) {
        for (const ResourceId existing: m_ids) {
            if (existing == id) {
                return;
            }
        }
        m_ids.push_back(id);
    }
}
