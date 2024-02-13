#pragma once
#include <memory>
#include <string>

namespace TechEngine {
    class MeshRendererComponent;
}

namespace TechEngineAPI {
    class Material;
    enum class MeshType {
        Cube,
        Sphere,
        Cylinder,
        Custom
    };

    class MeshRendererComponent {
    private:
        TechEngine::MeshRendererComponent* meshRendererComponent = nullptr;
        std::shared_ptr<Material> material;

    public:
        MeshRendererComponent(TechEngine::MeshRendererComponent* meshRendererComponent);

        void changeMesh(MeshType meshType);

        void changeMesh(MeshType meshType, std::string& filePath);

        void changeMaterial(std::shared_ptr<Material> material);

        std::shared_ptr<Material> getMaterial();
    };
}
