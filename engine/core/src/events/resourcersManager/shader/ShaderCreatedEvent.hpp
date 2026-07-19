#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class CORE_DLL ShaderCreatedEvent : public Event {
    private:
        const UUID uuid;

    public:
        explicit ShaderCreatedEvent(const UUID uuid) : uuid(uuid) {
        }

        const UUID& getUUID() const {
            return uuid;
        }
    };
}
