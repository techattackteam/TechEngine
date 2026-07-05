#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class CORE_DLL MaterialCreatedEvent : public Event {
    private:
        const UUID uuid;

    public:
        explicit MaterialCreatedEvent(const UUID uuid) : uuid(uuid) {
        }

        const UUID& getUUID() const {
            return uuid;
        }
    };
}
