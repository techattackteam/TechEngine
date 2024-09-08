#pragma once

#include "Vertex.hpp"

#include "serialization/FileStream.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include <filesystem>

namespace TechEngine {
    class AssimpLoader {
    public:
        struct aiMeshData {
            std::vector<Vertex> vertices; // Vertices of the mesh from Vertex.hpp Might change to internal format
            std::vector<int> indices;
            std::string material;

            static void Serialize(StreamWriter* writer, const aiMeshData& meshData) {
                writer->writeArray(meshData.vertices);
                writer->writeArray(meshData.indices);
            }

            static void Deserialize(StreamReader* reader, aiMeshData& meshData) {
                reader->readArray(meshData.vertices);
                reader->readArray(meshData.indices);
            }
        };

        struct Node {
            std::vector<Node> children;
            std::vector<aiMeshData> meshes;
        };

        struct ModelData {
            AssimpLoader::Node rootNode;
            std::vector<std::string> materials;
        };

    public:
        std::filesystem::path createStaticMeshFile(const Node& node, const std::string& staticMeshPath);

        AssimpLoader::ModelData loadModel(const std::string& path);

    private:
        ModelData processScene(const std::string& modelName, aiNode* aiNode, const aiScene* aiScene);

        aiMeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);
    };
}
