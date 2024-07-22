#pragma once

#include <string>

#include "material/Material.hpp"

namespace TechEngine {
    class MaterialManager;
}

namespace TechEngine {
    class CORE_DLL MaterialManagerAPI {
    private:
        inline static MaterialManager* materialManager = nullptr;

    public:
        explicit MaterialManagerAPI(MaterialManager* materialManager);

        static Material* createMaterial(std::string& name);

        static Material* copyMaterial(std::string& name, Material* material);

        static Material* createMaterial(std::string& name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        static bool deleteMaterial(std::string& name);

        static Material* getMaterial(std::string& name);
    };
}
