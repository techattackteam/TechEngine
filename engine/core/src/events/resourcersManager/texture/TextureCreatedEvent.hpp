#pragma once

#include "TechEngine/core/events/Event.hpp"
#include "TechEngine/core/core/CoreExportDLL.hpp"

namespace TechEngine {
    class CORE_DLL TextureCreatedEvent : public Event {
    private:
        const UUID m_uuid;

    public:
        TextureCreatedEvent(const UUID uuid) : m_uuid(uuid), Event() {
        }

        const UUID& getUUID() const {
            return m_uuid;
        }
    };
}
