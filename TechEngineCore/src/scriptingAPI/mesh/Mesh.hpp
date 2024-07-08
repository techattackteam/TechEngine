#pragma once

#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class Mesh;
}

namespace TechEngineAPI {
    class CORE_DLL Mesh {
    private:
        TechEngine::Mesh* mesh = nullptr;

    public:
        Mesh(TechEngine::Mesh* mesh);
    };
}
