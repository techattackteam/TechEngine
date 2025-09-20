#pragma once

#include "glm/glm.hpp"
#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"

namespace TechEngine {
    class Vertex {
    public:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;

        Vertex() = default;

        Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoords)
            : position(position), normal(normal), texCoords(texCoords) {
        }

        static void Serialize(StreamWriter* stream, const Vertex& vertex) {
            stream->writeRaw(vertex.position);
            stream->writeRaw(vertex.normal);
            stream->writeRaw(vertex.texCoords);
        }

        static void Deserialize(StreamReader* stream, Vertex& vertex) {
            stream->readRaw(vertex.position);
            stream->readRaw(vertex.normal);
            stream->readRaw(vertex.texCoords);
        }
    };
}
