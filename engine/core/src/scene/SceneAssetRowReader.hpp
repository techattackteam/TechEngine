#pragma once

#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>

#include <scene/SceneAssetEntity.hpp>

#include <span>

namespace TechEngine {
    class SceneAssetRowReader final : public ReferenceVisitor {
    private:
        std::span<const SceneAssetEntity> m_entities;
        bool m_malformed = false;

    public:
        explicit SceneAssetRowReader(std::span<const SceneAssetEntity> entities);

        bool isMalformed() const;

        void visitEntity(Entity& value) override;

        void visitAsset(ResourceId id, ResourceSlotId& handle) override;
    };
}
