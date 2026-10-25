#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>

#include <unordered_set>
#include <vector>

namespace TechEngine {
    class SceneAssetIdCollector final : public ReferenceVisitor {
    private:
        std::vector<ResourceId> m_ids;
        std::unordered_set<ResourceId> m_seen;

    public:
        const std::vector<ResourceId>& getIds() const;

        void visitEntity(Entity& value) override;

        void visitAsset(ResourceId id, ResourceSlotId& handle) override;
    };
}
