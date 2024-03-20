#include <string>
#include <utility>
#include "Material.hpp"

#include "event/EventDispatcher.hpp"
#include "events/material/MaterialUpdateEvent.hpp"

namespace TechEngine {
    Material::Material(std::string name, glm::vec4 color, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float shininess) : name(std::move(name)), color(color), ambient(ambient), diffuse(diffuse), specular(specular), shininess(shininess) {
    }

    Material::Material(const std::string& name, Texture* diffuse) : name(name), diffuseTexture(diffuse) {
        useTexture = true;
    }

    const std::string& Material::getName() {
        return name;
    }

    glm::vec4 Material::getColor() {
        return color;
    }

    void Material::setColor(glm::vec4 color) {
        this->color = color;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }

    glm::vec3 Material::getAmbient() {
        return ambient;
    }

    void Material::setAmbient(glm::vec3 ambient) {
        this->ambient = ambient;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }

    glm::vec3 Material::getDiffuse() {
        return diffuse;
    }

    void Material::setDiffuse(glm::vec3 diffuse) {
        this->diffuse = diffuse;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }

    glm::vec3 Material::getSpecular() {
        return specular;
    }

    void Material::setSpecular(glm::vec3 specular) {
        this->specular = specular;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }

    float Material::getShininess() const {
        return shininess;
    }

    void Material::setShininess(float shininess) {
        this->shininess = shininess;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }


    bool Material::getUseTexture() const {
        return useTexture;
    }

    void Material::setUseTexture(bool useTexture) {
        this->useTexture = useTexture;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }

    Texture* Material::getDiffuseTexture() {
        return diffuseTexture;
    }

    void Material::setDiffuseTexture(Texture* texture) {
        diffuseTexture = texture;
        EventDispatcher::getInstance().dispatch(new MaterialUpdateEvent(*this));
    }
}
