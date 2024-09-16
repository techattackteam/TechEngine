#pragma once
#include <string>
#include <utility>
#include <vector>

#include "Vertex.hpp"

namespace TechEngine {
    class Material;

    class Mesh {
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


        /*
        Mesh& operator+=(const Mesh& mesh) {
            this->vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            int lastIndex = indices.back();
            for (int index: mesh.indices) {
                this->indices.push_back(index + lastIndex);
            }
            return *this;
        }*/

        static void Serialize(StreamWriter* writer, const Mesh& mesh) {
            writer->writeArray(mesh.m_vertices);
            writer->writeArray(mesh.m_indices);
        }

        static void Deserialize(StreamReader* reader, Mesh& mesh) {
            reader->readArray(mesh.m_vertices);
            reader->readArray(mesh.m_indices);
        }

        const std::string& getName() const {
            return m_name;
        }
    };
}
