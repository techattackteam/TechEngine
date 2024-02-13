#pragma once
#include <string>
#include <glm/glm.hpp>


namespace TechEngine {
    class Material;
}

namespace TechEngineAPI {
    class Material {
    private:
        friend class MeshRendererComponent;
        friend class MaterialManagerAPI;
        TechEngine::Material* material = nullptr;

    public:
        explicit Material(TechEngine::Material* material);

        void setTexture(std::string& filePath);

        void changeColor(glm::vec3 color);

        void changeColor(glm::vec4 color);
    };
}
