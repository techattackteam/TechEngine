#pragma once

#include "resources/Mesh.hpp"
#include "resources/Material.hpp"

namespace TechEngine {
    class ResourcesManager;
}

namespace TechEngineAPI {
    class API_DLL Resources {
    private:
        inline static TechEngine::ResourcesManager* resourcesManager;

    public:
        Resources() = default;

        virtual ~Resources() = default;

        static void init(TechEngine::ResourcesManager* resourcesManager);

#pragma region Mesh
        static std::shared_ptr<Mesh> createMesh(const std::string& name);

        static std::shared_ptr<Mesh> getMesh(const std::string& name);
#pragma endregion

#pragma region Material
        static std::shared_ptr<Material> createMaterial(const std::string& name);

        static std::shared_ptr<Material> getMaterial(const std::string& name);
#pragma endregion
    };
}
