#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>

#include <unordered_map>

namespace TechEngine {
    class SceneAssetHandleFiller final : public ReferenceVisitor {
    private:
        const std::unordered_map<ResourceId, ResourceSlotId>& m_slots;

    public:
        explicit SceneAssetHandleFiller(const std::unordered_map<ResourceId, ResourceSlotId>& slots);

        void visitEntity(Entity& value) override;

        void visitAsset(ResourceId id, ResourceSlotId& handle) override;
    };
}
