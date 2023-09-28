#pragma once

#include "glm/glm.hpp"

class Vertex {
private:
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textCoords;
    glm::vec4 color;
public:
    inline Vertex(glm::vec3 position, glm::vec3 normal) : position(position), normal(normal), textCoords(0, 0) {

    };

    inline glm::vec3 getPosition() {
        return this->position;
    }

    inline void setColor(glm::vec4 color) {
        this->color = color;
    }

};
