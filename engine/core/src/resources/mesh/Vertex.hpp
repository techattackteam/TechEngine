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
        glm::vec4 color;

        Vertex() = default;

        Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoords)
            : position(position), normal(normal), texCoords(texCoords) {
        }

        Vertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& texCoords, const glm::vec4& color)
            : position(position), normal(normal), texCoords(texCoords), color(color) {
        }

        void setColor(glm::vec4& color) {
            this->color = color;
        }

        static void Serialize(StreamWriter* stream, const Vertex& vertex) {
            stream->writeRaw(vertex.position);
            stream->writeRaw(vertex.normal);
            stream->writeRaw(vertex.texCoords);
            stream->writeRaw(vertex.color);
        }

        static void Deserialize(StreamReader* stream, Vertex& vertex) {
            stream->readRaw(vertex.position);
            stream->readRaw(vertex.normal);
            stream->readRaw(vertex.texCoords);
            stream->readRaw(vertex.color);
        }
    };
}
