#pragma once

#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/ReferenceVisitor.hpp>
#include <TechEngine/core/serialization/Visit.hpp>
#include <TechEngine/core/serialization/Writer.hpp>

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
        void (*writePayload)(Writer& writer, const void* component) = nullptr;
        void (*readPayload)(Reader& reader, void* component) = nullptr;
        void (*visitReferences)(ReferenceVisitor& visitor, void* component) = nullptr;
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

        // A visit that is not visible here registers T as not serializable, silently. Define it
        // in T's own header, or as a hidden friend, so every registering TU sees the same answer.
        template<ComponentValue T>
        ComponentTypeId registerComponent(const std::string_view tag) {
            ComponentTypeId id;
            if constexpr (Visitable<T, Writer> && Visitable<T, Reader> && Visitable<T, ReferenceVisitor>) {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, &writeComponentPayload<T>, &readComponentPayload<T>, &visitComponentReferences<T>);
            } else if constexpr (Visitable<T, Writer> && Visitable<T, Reader>) {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, &writeComponentPayload<T>, &readComponentPayload<T>, nullptr);
            } else {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, nullptr, nullptr, nullptr);
            }
            internal::g_componentTypeSlot<T> = id;
            return id;
        }

        const ComponentTypeRecord* find(ComponentTypeId id) const;

        const ComponentTypeRecord* find(ComponentDenseId id) const;

        ComponentDenseId denseId(ComponentTypeId id) const;

        std::span<const ComponentTypeRecord> records() const;

        void freeze();

        bool frozen() const;

        std::size_t typeCount() const;

    private:
        ComponentTypeId registerType(std::string_view tag, ComponentTypeId existingTypeId, std::unique_ptr<IComponentStorage> (*createStorage)(), void (*writePayload)(Writer&, const void*), void (*readPayload)(Reader&, void*), void (*visitReferences)(ReferenceVisitor&, void*));

        template<ComponentValue T>
        static void writeComponentPayload(Writer& writer, const void* component) {
            writer.field(*const_cast<T*>(static_cast<const T*>(component)));
        }

        template<ComponentValue T>
        static void readComponentPayload(Reader& reader, void* component) {
            reader.field(*static_cast<T*>(component));
        }

        template<ComponentValue T>
        static void visitComponentReferences(ReferenceVisitor& visitor, void* component) {
            visitor.field(*static_cast<T*>(component));
        }
    };
}
