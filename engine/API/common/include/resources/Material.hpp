#pragma once

#include "common/include/core/ExportDLL.hpp"
#include <string>
#include <glm/glm.hpp>

namespace TechEngine {
    class ResourcesManager;
}

namespace TechEngineAPI {
    class API_DLL Material {
    private:
        friend class Resources;
        TechEngine::ResourcesManager* m_resourcesManager;
        std::string m_name;

        glm::vec3 m_ambient;
        glm::vec3 m_diffuse;
        glm::vec3 m_specular;

        glm::vec4 m_color;
        float m_shininess;

        bool useTexture = false;


    public:
        Material(const std::string& name, TechEngine::ResourcesManager* resourcesManager);
        const std::string& getName() const;

        const glm::vec4& getColor() const;

        const glm::vec3& getAmbient() const;

        const glm::vec3& getDiffuse() const;

        const glm::vec3& getSpecular() const;

        const float getShininess() const;

        void setColor(const glm::vec4& color);

        void setAmbient(const glm::vec3& ambient);

        void setDiffuse(const glm::vec3& diffuse);

        void setSpecular(const glm::vec3& specular);
    };
}
