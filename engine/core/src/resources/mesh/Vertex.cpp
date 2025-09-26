#include "TechEngine/core/resources/mesh/Vertex.hpp"

#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"

namespace TechEngine {
    void Vertex::Serialize(StreamWriter* stream, const Vertex& vertex) {
        stream->writeRaw(vertex.position);
        stream->writeRaw(vertex.normal);
        stream->writeRaw(vertex.texCoords);
    }

    void Vertex::Deserialize(StreamReader* stream, Vertex& vertex) {
        stream->readRaw(vertex.position);
        stream->readRaw(vertex.normal);
        stream->readRaw(vertex.texCoords);
    }
}
