#pragma once

#include "Vertex.hpp"
#include "resources/IResource.hpp"

namespace TechEngine {
    class CORE_DLL Mesh : public IResource {
    public:
        std::string m_name;

        std::vector<Vertex> m_vertices;
        std::vector<int> m_indices;

        uint32_t indexCount = 0; // Number of indices to draw
        uint32_t firstIndex = 0; // Offset in the main IBO (in number of indices)
        uint32_t baseVertex = 0; // Offset in the main VBO (in number of vertices)

        Mesh(std::string name,
             std::vector<Vertex> vertices,
             std::vector<int> indices);

        Mesh(const Mesh& rhs);

        void setVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices);

        void addVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices);

        static void Serialize(StreamWriter* writer, const Mesh& mesh) {
            writer->writeArray(mesh.m_vertices);
            writer->writeArray(mesh.m_indices);
        }

        static void Deserialize(StreamReader* reader, Mesh& mesh) {
            reader->readArray(mesh.m_vertices);
            reader->readArray(mesh.m_indices);
        }

        const std::string& getName() const;
    };
}
