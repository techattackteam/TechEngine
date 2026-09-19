#pragma once

#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    struct ComponentTypeRecord {
        ComponentTypeId id;
        ComponentDenseId denseId;
        std::string tag;
        std::unique_ptr<IComponentStorage> (*createStorage)();
    };

    class ComponentRegistry {
    private:
        std::vector<ComponentTypeRecord> m_componentRecords;
        std::unordered_map<ComponentTypeId, std::uint16_t> m_indexById;
        std::uint32_t m_typeLimit = ComponentDenseId::VALUE_COUNT;
        bool m_frozen = false;

    public:
        explicit ComponentRegistry(std::uint32_t typeLimit = ComponentDenseId::VALUE_COUNT);

        ComponentRegistry(const ComponentRegistry&) = delete;

        ComponentRegistry& operator=(const ComponentRegistry&) = delete;

        template<ComponentValue T>
        ComponentTypeId registerComponent(const std::string_view tag) {
            const ComponentTypeId id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>);
            internal::g_componentTypeSlot<T> = id;
            return id;
        }

        const ComponentTypeRecord* find(ComponentTypeId id) const;

        const ComponentTypeRecord* find(ComponentDenseId id) const;

        ComponentDenseId denseId(ComponentTypeId id) const;

        std::span<const ComponentTypeRecord> records() const;

        void freeze();

        bool frozen() const;

        std::string_view tagOf(ComponentTypeId id) const;

        std::size_t typeCount() const;

    private:
        ComponentTypeId registerType(std::string_view tag, ComponentTypeId existingTypeId, std::unique_ptr<IComponentStorage> (*createStorage)());
    };
}
