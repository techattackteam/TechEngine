#pragma once

#include <glm/glm.hpp>
#include "renderer/Texture.hpp"

namespace TechEngine {

    class Material {
    private:

        std::string name;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        glm::vec4 color;
        float shininess;

        bool useTexture = false;

        Texture *diffuseTexture = nullptr;

    public:
        Material(std::string name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        Material(const std::string &name, Texture *diffuse);

        const std::string &getName();

        glm::vec4 &getColor();

        glm::vec3 &getAmbient();

        glm::vec3 &getDiffuse();

        glm::vec3 &getSpecular();

        float &getShininess();

        bool &getUseTexture();

        Texture *getDiffuseTexture();

        void setDiffuseTexture(Texture *texture);
    };
}
