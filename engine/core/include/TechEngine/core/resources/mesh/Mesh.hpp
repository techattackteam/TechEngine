#pragma once

#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "TechEngine/core/resources/IResource.hpp"

namespace TechEngine {
    class StreamReader;
    class StreamWriter;

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

        static void Serialize(StreamWriter* writer, const Mesh& mesh);

        static void Deserialize(StreamReader* reader, Mesh& mesh);

        const std::string& getName() const;
    };
}
