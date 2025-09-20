#pragma once

#include "resources/IResource.hpp"

#include <glm/glm.hpp>

namespace TechEngine {
    struct MaterialProperties {
        glm::vec4 color;
    };

    class CORE_DLL Material : public IResource {
    private:
        std::string name;
        uint32_t m_gpuID; // ID in the GPU

        MaterialProperties properties;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;

    public:
        Material(std::string name, uint32_t gpuID, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        std::string& getName();

        uint32_t getGpuID() const;

        glm::vec4& getColor();

        void setColor(glm::vec4 color);

        glm::vec3 getAmbient();

        void setAmbient(glm::vec3 ambient);

        glm::vec3 getDiffuse();

        void setDiffuse(glm::vec3 diffuse);

        glm::vec3 getSpecular();

        void setSpecular(glm::vec3 specular);

        float getShininess();

        void setShininess(float shininess);

        MaterialProperties& getProperties();
    };
}
