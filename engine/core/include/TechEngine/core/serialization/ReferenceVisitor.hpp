#pragma once

#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/serialization/Visit.hpp>

#include <concepts>
#include <string_view>

namespace TechEngine {
    template<typename T>
    concept ReferenceField = std::same_as<T, Entity> || AssetReference<T>;

    class ReferenceVisitor {
    public:
        virtual ~ReferenceVisitor() = default;

        virtual void visitEntity(Entity& value) = 0;

        virtual void visitAsset(ResourceId id, ResourceSlotId& handle) = 0;

        template<typename T>
        void field(T& value) {
            if constexpr (Visitable<T, ReferenceVisitor>) {
                visit(*this, value);
            }
        }

        template<typename T>
        void field(std::string_view, T& value) {
            field(value);
        }

        void field(Entity& value) {
            if (value.valid()) {
                visitEntity(value);
            }
        }

        template<typename T>
        void field(AssetRef<T>& value) {
            if (value.id.valid()) {
                ResourceSlotId slot = value.handle.slot();
                visitAsset(value.id, slot);
                value.handle = ResourceHandle<T>{slot.index, slot.generation};
            }
        }
    };
}
