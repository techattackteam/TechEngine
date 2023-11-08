#include <string>
#include "Material.hpp"

namespace TechEngine {

    Material::Material(const std::string &name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) :
            name(name), color(color), ambient(ambient), diffuse(diffuse), specular(specular), shininess(shininess) {

    }

    const std::string &Material::getName() {
        return name;
    }

    glm::vec4 &Material::getColor() {
        return color;
    }

    glm::vec3 &Material::getAmbient() {
        return ambient;
    }

    glm::vec3 &Material::getDiffuse() {
        return diffuse;
    }

    glm::vec3 &Material::getSpecular() {
        return specular;
    }

    float &Material::getShininess() {
        return shininess;
    }


}

