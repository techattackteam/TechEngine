#pragma once

#include "core/CoreExportDLL.hpp"
#include <glm/glm.hpp>

namespace TechEngine {
    class CORE_DLL Material {
    private:
        std::string name;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        glm::vec4 color;
        float shininess;

        bool useTexture = false;

        /*Texture* diffuseTexture = nullptr;*/

    public:
        Material(std::string name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        /*Material(const std::string& name, Texture* diffuse);*/

        const std::string& getName();

        glm::vec4 getColor();

        void setColor(glm::vec4 color);

        glm::vec3 getAmbient();

        void setAmbient(glm::vec3 ambient);

        glm::vec3 getDiffuse();

        void setDiffuse(glm::vec3 diffuse);

        glm::vec3 getSpecular();

        void setSpecular(glm::vec3 specular);

        float getShininess() const;

        void setShininess(float shininess);

        bool getUseTexture() const;

        void setUseTexture(bool useTexture);

        /*Texture* getDiffuseTexture();

        void setDiffuseTexture(Texture* texture);*/
    };
}
