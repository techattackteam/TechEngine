#pragma once
#include "core/CoreExportDLL.hpp"
#include "ComponentStorage.hpp"
#include "components/Entity.hpp"
#include <functional>
#include <memory>
#include <span>

namespace TechEngine {
    class Archetype;
    using ComponentTypeID = uint32_t;
    using ArchetypeID = uint32_t;

    class Component {
    public:
        inline static ComponentTypeID familyCounter = 0;
    };

    struct ComponentInfo {
        size_t size;
        size_t alignment;
        std::function<std::unique_ptr<IComponentStorage>()> createStorage;
    };

    template<typename T>
    class ComponentType {
    public:
        static ComponentTypeID get() {
            static ComponentTypeID typeID = Component::familyCounter++;
            return typeID;
        }
    };

    struct ComponentMapping {
        IComponentStorage* source_storage;
        IComponentStorage* destination_storage;
    };

    struct ArchetypeEdge {
        Archetype* destinationArchetype;
        std::vector<ComponentMapping> componentMappings;
    };

    class CORE_DLL Archetype {
    private:
        friend class ArchetypesManager;
        friend class SceneSerializer;
        ArchetypeID m_id;
        size_t m_indexInECS;
        std::vector<Entity> m_entities; // List of entities in this archetype
        std::vector<ComponentTypeID> m_componentTypes;
        std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentStorage>> m_componentData; // Storage for component data

        std::unordered_map<ComponentTypeID, ArchetypeEdge> m_addTransition;
        std::unordered_map<ComponentTypeID, ArchetypeEdge> m_removeTransition;

    public:
        explicit Archetype(ArchetypeID id, size_t index, const std::vector<ComponentTypeID>& types) : m_id(id), m_indexInECS(index), m_componentTypes(types) {
            m_componentTypes = types;
        }

        Archetype(const Archetype&) = delete;

        Archetype& operator=(const Archetype&) = delete;

        // You should still define move operations if moving is allowed
        Archetype(Archetype&&) noexcept = default;

        Archetype& operator=(Archetype&&) noexcept = default;


        bool operator==(const Archetype& lhr) const {
            if (m_id != lhr.m_id) {
                return false;
            } else {
                return true;
            }
        }

        // Method to get the component types for this archetype
        std::vector<ComponentTypeID> getComponentTypes() const;

        template<typename T>
        std::span<T> getComponentArrayAsRawPtr() {
            ComponentTypeID typeID = ComponentType<T>::get();
            auto* storagePtr = m_componentData.at(typeID).get();
            void* rawData = storagePtr->getData();
            T* typed_data = static_cast<T*>(rawData);

            // Return a span covering the entire array of components.
            return std::span<T>(typed_data, m_entities.size());
        }

        const std::vector<Entity>& getEntities() const;

        void reserve(size_t count);

    private:
        void addEntity(Entity entity);

        Entity removeEntity(size_t index);

        template<typename T>
        T& getComponent(size_t index) {
            ComponentTypeID typeID = ComponentType<T>::get();
            auto* componentStorage = static_cast<ComponentStorage<T>*>(m_componentData.at(typeID).get());
            return componentStorage->get(index);
        }


        size_t getComponentSize(ComponentTypeID typeID);

        bool containsComponents(const std::vector<ComponentTypeID>& componentTypes);

        size_t reserveSlotFor(Entity entity);

        template<typename T>
        void setComponentData(size_t index, const T& component) {
            ComponentTypeID typeID = ComponentType<T>::get();
            auto* storage = static_cast<ComponentStorage<T>*>(m_componentData.at(typeID).get());
            storage->set(index, component);
        }
    };
}
