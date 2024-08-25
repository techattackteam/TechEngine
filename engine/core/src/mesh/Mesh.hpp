#pragma once
#include <string>
#include <vector>

#include "Vertex.hpp"

namespace TechEngine {
    class Mesh {
    public:
        std::string m_name;

        std::vector<Vertex> vertices;
        std::vector<int> indices;
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
            writer->writeArray(mesh.vertices);
            writer->writeArray(mesh.indices);
        }

        static void Deserialize(StreamReader* reader, Mesh& mesh) {
            reader->readArray(mesh.vertices);
            reader->readArray(mesh.indices);
        }
    };
}
