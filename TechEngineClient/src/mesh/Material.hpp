#pragma once

#include <glm/glm.hpp>

namespace TechEngine {

    class Material {
    private:

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        glm::vec4 color;
        float shininess;

    public:
        Material(glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess);

        glm::vec4 &getColor();

        glm::vec3 &getAmbient();

        glm::vec3 &getDiffuse();

        glm::vec3 &getSpecular();

        float &getShininess();

    };
}
