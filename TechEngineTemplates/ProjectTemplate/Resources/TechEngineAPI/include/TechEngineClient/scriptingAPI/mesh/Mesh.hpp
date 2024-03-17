#pragma once


namespace TechEngine {
    class Mesh;
}

namespace TechEngineAPI {
    class Mesh {
    private:
        TechEngine::Mesh* mesh = nullptr;

    public:
        Mesh(TechEngine::Mesh* mesh);
    };
}
