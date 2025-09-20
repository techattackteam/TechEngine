#pragma once

#include "core/CoreExportDLL.hpp"
#include "events/Event.hpp"

namespace TechEngine {
    class CORE_DLL MaterialCreatedEvent : public Event {
    private:
        std::string name;

    public:
        explicit MaterialCreatedEvent(std::string name) : name(std::move(name)) {
        }

        [[nodiscard]] const std::string& getName() const {
            return name;
        }
    };
}
