#pragma once

#include "Vertex.hpp"

#include "serialization/FileStream.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include <filesystem>

namespace TechEngine {
    class AssimpLoader {
    public:
        struct MeshData {
            std::vector<Vertex> vertices; // Vertices of the mesh from Vertex.hpp Might change to internal format
            std::vector<int> indices;
            std::string material;

            static void Serialize(StreamWriter* writer, const MeshData& meshData) {
                writer->writeArray(meshData.vertices);
                writer->writeArray(meshData.indices);
            }

            static void Deserialize(StreamReader* reader, MeshData& meshData) {
                reader->readArray(meshData.vertices);
                reader->readArray(meshData.indices);
            }
        };

        struct Node {
            std::vector<Node> children;
            std::vector<MeshData> meshes;
        };

    public:
        Node loadModel(const std::string& path);
    private:
        Node processScene(const std::string& modelName, ::aiNode* aiNode, const aiScene* aiScene, bool _return = false);

        MeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);
    };
}
