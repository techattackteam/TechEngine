#include "Material.hpp"

Material::Material(glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) :
        color(color), ambient(ambient), diffuse(diffuse), specular(specular), shininess(shininess) {

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

float Material::getShininess() {
    return shininess;
}

