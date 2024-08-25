#pragma once

#include "Vertex.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"

#include <filesystem>

#include "serialization/FileStream.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class AssimpLoader : public System { //TODO: Remove from System when Resource Manager is implemented
    private:
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
        std::filesystem::path createStaticMeshFile(const std::string& path); // Loads model (eg obj, fbx) and creates a static TE_mesh file

        //Create prefab from model

    private:
        AssimpLoader::Node loadModel(const std::string& path);

        Node processScene(const std::string& modelName, ::aiNode* aiNode, const aiScene* aiScene, bool _return = false);

        MeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);
    };
}
