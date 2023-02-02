#include <vector>
#include <glm/glm.hpp>

#include "GLFW.hpp"

#pragma once

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textCoords;
    glm::vec4 color;

    inline Vertex(glm::vec3 position, glm::vec3 normal, glm::vec4 color) : position(position), normal(normal), textCoords(0, 0), color(color) {

    };

};

class VertexBuffer {
private:
    uint32_t id;
    uint32_t offset = 0;

public:
    VertexBuffer() = default;

    ~VertexBuffer();

    void init(uint32_t id, uint32_t size);

    void bind() const;

    void unBind() const;

    void addData(const void *data, uint32_t size, uint32_t offset) const;

    uint32_t getOffset();

    void setOffset(int offset);
};