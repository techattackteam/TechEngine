#pragma once

#include "glm/glm.hpp"

class Vertex {
private:
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textCoords;
    glm::vec4 color;
public:
    inline Vertex(glm::vec3 position, glm::vec3 normal, glm::vec2 textureCoordinate) : position(position), normal(normal), textCoords(textureCoordinate) {

    };

    inline glm::vec3 getPosition() {
        return this->position;
    }

    inline glm::vec3 getNormal() {
        return this->normal;
    }

    inline glm::vec2 getTextureCoordinate() {
        return this->textCoords;
    }

    inline glm::vec3 getColor() {
        return this->color;
    }

    inline void setColor(glm::vec4 color) {
        this->color = color;
    }

};
