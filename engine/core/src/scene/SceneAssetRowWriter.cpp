#include <TechEngine/base/diagnostics/Log.hpp>

#include <scene/SceneAssetRowWriter.hpp>

namespace TechEngine {
    SceneAssetRowWriter::SceneAssetRowWriter(const std::unordered_map<Entity, std::uint32_t>& rows, const ResourceId sceneId) : m_rows(rows), m_sceneId(sceneId) {
    }

    void SceneAssetRowWriter::visitEntity(Entity& value) {
        const auto row = m_rows.find(value);
        if (row != m_rows.end()) {
            value = Entity{row->second, 0};
        } else {
            TE_LOGGER_WARN("Scene asset {0:#018x} has a reference to an entity that is not in the scene; rewriting to empty", m_sceneId.value());
            value = Entity{};
        }
    }

    void SceneAssetRowWriter::visitAsset(ResourceId, ResourceSlotId&) {
    }
}
