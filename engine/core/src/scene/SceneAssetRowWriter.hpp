#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>

#include <cstdint>
#include <unordered_map>

namespace TechEngine {
    class SceneAssetRowWriter final : public ReferenceVisitor {
    private:
        const std::unordered_map<Entity, std::uint32_t>& m_rows;
        ResourceId m_sceneId;

    public:
        SceneAssetRowWriter(const std::unordered_map<Entity, std::uint32_t>& rows, ResourceId sceneId);

        void visitEntity(Entity& value) override;

        void visitAsset(ResourceId id, ResourceSlotId& handle) override;
    };
}
