#pragma once

#include "../Event.hpp"
#include "TechEngine/core/components/Archetype.hpp"

namespace TechEngine {
    class CORE_DLL ComponentRemovedEvent : public Event {
    private:
        Entity m_entity = -1;
        ComponentTypeID m_componentType = -1;

    public:
        explicit ComponentRemovedEvent(Entity entity, ComponentTypeID componentTypeID) : m_entity(entity), m_componentType(componentTypeID) {
        }

        Entity getEntity() const {
            return m_entity;
        }

        ComponentTypeID getComponentTypeID() const {
            return m_componentType;
        }
    };
}
