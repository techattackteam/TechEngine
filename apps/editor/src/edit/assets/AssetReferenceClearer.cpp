#include <edit/assets/AssetReferenceClearer.hpp>

#include <algorithm>

namespace TechEngine {
    AssetReferenceClearer::AssetReferenceClearer(const std::span<const ResourceId> ids) : m_ids(ids) {
    }

    std::size_t AssetReferenceClearer::getClearedCount() const {
        return m_clearedCount;
    }

    void AssetReferenceClearer::visitEntity(Entity&) {
    }

    void AssetReferenceClearer::visitAsset(ResourceId& id, ResourceSlotId& handle) {
        if (std::ranges::find(m_ids, id) == m_ids.end()) {
            return;
        }
        id = ResourceId{};
        handle = ResourceSlotId{};
        m_clearedCount++;
    }
}
