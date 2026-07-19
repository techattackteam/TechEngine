#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class CORE_DLL ShaderDeletedEvent : public Event {
    private:
        const UUID uuid;

    public:
        explicit ShaderDeletedEvent(const UUID uuid) : uuid(uuid) {
        }

        const UUID& getUUID() const {
            return uuid;
        }
    };
}
