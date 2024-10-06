#pragma once
#include <string>
#include <utility>
#include <vector>

#include "Vertex.hpp"

namespace TechEngine {
    class Material;

    class CORE_DLL Mesh {
    public:
        std::string m_name;

        std::vector<Vertex> m_vertices;
        std::vector<int> m_indices;

        Mesh(std::string name,
             std::vector<Vertex> vertices,
             std::vector<int> indices) : m_name(std::move(name)),
                                         m_vertices(std::move(vertices)),
                                         m_indices(std::move(indices)) {
        }

        Mesh(const Mesh& rhs);

        /*
        Mesh& operator=(Mesh rhs);
        */

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
