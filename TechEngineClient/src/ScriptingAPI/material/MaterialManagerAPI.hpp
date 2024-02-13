#pragma once

#include <memory>
#include <string>

#include "Material.hpp"


namespace TechEngine {
    class MaterialManager;
}

namespace TechEngineAPI {
    class MaterialManagerAPI {
    private:
        inline static TechEngine::MaterialManager* materialManager = nullptr;

        static std::shared_ptr<Material> copyMaterial(std::string& name, TechEngine::Material* material);

    public:
        explicit MaterialManagerAPI(TechEngine::MaterialManager* materialManager);

        static std::shared_ptr<Material> createMaterial(std::string& name);

        static std::shared_ptr<Material> copyMaterial(std::string& name, const std::shared_ptr<Material>& basedMaterial);

        static std::shared_ptr<Material> createMaterial(std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        static bool deleteMaterial(std::string& name);

        static std::shared_ptr<Material> getMaterial(std::string& name);
    };
}
