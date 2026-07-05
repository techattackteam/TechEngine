#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/core/UUID.hpp"

namespace TechEngine {
    class CORE_DLL MeshCreatedEvent : public Event {
    private:
        const UUID m_uuid;

    public:
        MeshCreatedEvent(const UUID uuid) : m_uuid(uuid), Event() {
        }

        const UUID& getUUID() const {
            return m_uuid;
        }
    };
}
