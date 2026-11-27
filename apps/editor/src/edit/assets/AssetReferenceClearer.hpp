#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>

#include <cstddef>
#include <span>

namespace TechEngine {
    class AssetReferenceClearer final : public ReferenceVisitor {
    private:
        std::span<const ResourceId> m_ids;
        std::size_t m_clearedCount = 0;

    public:
        // ids must outlive the walk.
        explicit AssetReferenceClearer(std::span<const ResourceId> ids);

        std::size_t getClearedCount() const;

        void visitEntity(Entity& value) override;

        void visitAsset(ResourceId& id, ResourceSlotId& handle) override;
    };
}
