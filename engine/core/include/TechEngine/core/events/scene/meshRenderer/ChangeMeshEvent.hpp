#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/core/resources/mesh/Mesh.hpp"
#include "TechEngine/core/components/Entity.hpp"

namespace TechEngine {
    class CORE_DLL ChangeMeshEvent : public Event {
    private:
        Entity m_entity = -1;
        Mesh* from;
        Mesh* to;

    public:
        explicit ChangeMeshEvent(Entity entity, Mesh* from, Mesh* to) : m_entity(entity), from(from), to(to) {
        }

        explicit ChangeMeshEvent() : m_entity(-1), from(nullptr), to(nullptr) {
        }

        const Entity& getEntity() const {
            return m_entity;
        }

        const Mesh* getFrom() const {
            return from;
        }

        const Mesh* getTo() const {
            return to;
        }
    };
}
