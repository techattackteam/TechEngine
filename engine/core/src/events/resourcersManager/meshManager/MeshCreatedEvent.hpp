#pragma once

#include "core/CoreExportDLL.hpp"
#include "events/Event.hpp"
#include "resources/mesh/Mesh.hpp"

namespace TechEngine {
    class CORE_DLL MeshCreatedEvent : public Event {
    public:
        std::string m_name;

        MeshCreatedEvent(std::string name) : m_name(name), Event() {
        }
    };
}
