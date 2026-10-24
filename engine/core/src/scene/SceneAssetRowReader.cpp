#include <scene/SceneAssetRowReader.hpp>

namespace TechEngine {
    SceneAssetRowReader::SceneAssetRowReader(const std::span<const SceneAssetEntity> entities) : m_entities(entities) {
    }

    bool SceneAssetRowReader::isMalformed() const {
        return m_malformed;
    }

    void SceneAssetRowReader::visitEntity(Entity& value) {
        if (value.index < m_entities.size()) {
            value = m_entities[value.index].entity;
        } else {
            m_malformed = true;
            value = Entity{};
        }
    }

    void SceneAssetRowReader::visitAsset(ResourceId, ResourceSlotId&) {
    }
}
