#pragma once

#include "core/CoreExportDLL.hpp"
#include "events/Event.hpp"

namespace TechEngine {
    class Mesh;

    class CORE_DLL MeshDeletedEvent : public Event {
    public:
        Mesh& mesh;

        MeshDeletedEvent(Mesh& mesh) : mesh(mesh) {
        }
    };
}
