#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include "TechEngine/core/events/Event.hpp"

namespace TechEngine {
    class Mesh;

    class CORE_DLL MeshDeletedEvent : public Event {
    public:
        Mesh& mesh;

        MeshDeletedEvent(Mesh& mesh) : mesh(mesh) {
        }
    };
}
