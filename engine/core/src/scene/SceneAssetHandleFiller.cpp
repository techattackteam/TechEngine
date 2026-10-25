#include "TechEngine/base/diagnostics/Log.hpp"
#include <scene/SceneAssetHandleFiller.hpp>

namespace TechEngine {
    SceneAssetHandleFiller::SceneAssetHandleFiller(const std::unordered_map<ResourceId, ResourceSlotId>& slots) : m_slots(slots) {
    }

    void SceneAssetHandleFiller::visitEntity(Entity&) {
    }

    void SceneAssetHandleFiller::visitAsset(ResourceId id, ResourceSlotId& out) {
        const auto slot = m_slots.find(id);
        if (slot != m_slots.end()) {
            out = slot->second;
        } else {
            TE_LOGGER_WARN("Scene asset has a reference to an asset {0:#018x} that is not in the scene; rewriting to empty", id.value());
            out = ResourceSlotId{};
        }
    }
}
