#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/core/UUID.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class CORE_DLL MaterialDeletedEvent : public Event {
    private:
        const UUID m_uuid;

    public:
        explicit MaterialDeletedEvent(const UUID uuid) : m_uuid(uuid), Event() {
        }

        const UUID& getUUID() const {
            return m_uuid;
        }
    };
}
