#pragma once

#include "TechEngine/core/events/Event.hpp"
#include  "TechEngine/core/components/Entity.hpp"

namespace TechEngine {
    class CORE_DLL EntityDeletedEvent : public Event {
    private:
        Entity m_entity;

    public:
        explicit EntityDeletedEvent(int entity) : m_entity(entity) {
        }

        [[nodiscard]] int getEntity() const {
            return m_entity;
        }
    };
}
