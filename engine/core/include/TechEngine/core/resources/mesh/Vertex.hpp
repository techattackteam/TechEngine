#pragma once

#include "glm/glm.hpp"

namespace TechEngine {
    class StreamReader;
    class StreamWriter;

    class Vertex {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;

        Vertex() = default;

        Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoords)
            : position(position), normal(normal), texCoords(texCoords) {
        }

        static void serialize(StreamWriter* stream, const Vertex& vertex);

        static void deserialize(StreamReader* stream, Vertex& vertex);
    };
}
