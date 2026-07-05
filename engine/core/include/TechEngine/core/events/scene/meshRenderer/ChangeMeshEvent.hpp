#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/core/resources/mesh/MeshResource.hpp"
#include "TechEngine/core/components/Entity.hpp"

namespace TechEngine {
    class CORE_DLL ChangeMeshEvent : public Event {
    private:
        Entity m_entity = -1;
        MeshResource* from;
        MeshResource* to;

    public:
        explicit ChangeMeshEvent(Entity entity, MeshResource* from, MeshResource* to) : m_entity(entity), from(from), to(to) {
        }

        explicit ChangeMeshEvent() : m_entity(-1), from(nullptr), to(nullptr) {
        }

        const Entity& getEntity() const {
            return m_entity;
        }

        const MeshResource* getFrom() const {
            return from;
        }

        const MeshResource* getTo() const {
            return to;
        }
    };
}
