#pragma once

#include <TechEngine/core/scene/ComponentStorage.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/serialization/FieldVisitor.hpp>
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
#include <utility>
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
        void (*visitFields)(FieldVisitor& visitor, void* component) = nullptr;
        void (*editFields)(FieldVisitor& visitor, void* component) = nullptr;
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
            void (*visitFields)(FieldVisitor&, void*) = nullptr;
            void (*editFields)(FieldVisitor&, void*) = nullptr;
            if constexpr (Visitable<T, FieldVisitor>) {
                visitFields = &visitComponentFields<T>;
                editFields = &editComponentFields<T>;
            }

            ComponentTypeId id;
            if constexpr (Visitable<T, Writer> && Visitable<T, Reader> && Visitable<T, ReferenceVisitor>) {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, &writeComponentPayload<T>, &readComponentPayload<T>, &visitComponentReferences<T>, visitFields, editFields);
            } else if constexpr (Visitable<T, Writer> && Visitable<T, Reader>) {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, &writeComponentPayload<T>, &readComponentPayload<T>, nullptr, visitFields, editFields);
            } else {
                id = registerType(tag, internal::g_componentTypeSlot<T>, &createComponentStorage<T>, nullptr, nullptr, nullptr, visitFields, editFields);
            }
            internal::g_componentTypeSlot<T> = id;
            return id;
        }

        const ComponentTypeRecord* find(ComponentTypeId id) const;

        const ComponentTypeRecord* find(ComponentDenseId id) const;

        ComponentDenseId getDenseId(ComponentTypeId id) const;

        std::span<const ComponentTypeRecord> records() const;

        void freeze();

        bool frozen() const;

        std::size_t typeCount() const;

    private:
        ComponentTypeId registerType(
            std::string_view tag,
            ComponentTypeId existingTypeId,
            std::unique_ptr<IComponentStorage> (*createStorage)(),
            void (*writePayload)(Writer&, const void*),
            void (*readPayload)(Reader&, void*),
            void (*visitReferences)(ReferenceVisitor&, void*),
            void (*visitFields)(FieldVisitor&, void*),
            void (*editFields)(FieldVisitor&, void*));

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

        template<ComponentValue T>
        static void visitComponentFields(FieldVisitor& visitor, void* component) {
            visit(visitor, *static_cast<T*>(component));
        }

        template<ComponentValue T>
        static void editComponentFields(FieldVisitor& visitor, void* component) {
            T copy = *static_cast<T*>(component);
            visit(visitor, copy);
            *static_cast<T*>(component) = std::move(copy);
        }
    };
}
