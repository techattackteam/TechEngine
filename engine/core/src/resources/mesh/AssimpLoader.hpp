#pragma once

#include "TechEngine/core/resources/mesh/Vertex.hpp"

#include "serialization/FileStream.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include <filesystem>
#include <glm/glm.hpp>

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
            std::string name;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 rotation = glm::vec3(0.0f); // Euler angles in degrees
            glm::vec3 scale = glm::vec3(1.0f);
            std::vector<Node> children;
            std::vector<aiMeshData> meshes;
        };

        struct ModelData {
            AssimpLoader::Node rootNode;
            std::vector<std::string> materials;
        };

        // Serializable node for .temodel file (references mesh files instead of containing mesh data)
        struct ModelNode {
            std::string name;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 rotation = glm::vec3(0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            std::vector<std::string> meshFiles; // Relative paths to .tesmesh files
            std::vector<ModelNode> children;

            static void Serialize(StreamWriter* writer, const ModelNode& node) {
                writer->writeString(node.name);
                writer->writeRaw(node.position);
                writer->writeRaw(node.rotation);
                writer->writeRaw(node.scale);
                writer->writeArray(node.meshFiles);
                writer->writeRaw<uint32_t>(static_cast<uint32_t>(node.children.size()));
                for (const auto& child : node.children) {
                    Serialize(writer, child);
                }
            }

            static void Deserialize(StreamReader* reader, ModelNode& node) {
                reader->readString(node.name);
                reader->readRaw(node.position);
                reader->readRaw(node.rotation);
                reader->readRaw(node.scale);
                reader->readArray(node.meshFiles);
                uint32_t childCount = 0;
                reader->readRaw<uint32_t>(childCount);
                node.children.resize(childCount);
                for (uint32_t i = 0; i < childCount; ++i) {
                    Deserialize(reader, node.children[i]);
                }
            }
        };

        // Data structure for .temodel file
        struct TEModelData {
            std::string modelName;
            ModelNode rootNode;

            static void Serialize(StreamWriter* writer, const TEModelData& data) {
                writer->writeString(data.modelName);
                ModelNode::Serialize(writer, data.rootNode);
            }

            static void Deserialize(StreamReader* reader, TEModelData& data) {
                reader->readString(data.modelName);
                ModelNode::Deserialize(reader, data.rootNode);
            }
        };

    public:
        // Creates a single merged .tesmesh file from all meshes in the hierarchy
        std::filesystem::path createStaticMeshFile(const Node& node, const std::string& staticMeshPath);

        // Creates a .temodel file and individual .tesmesh files in a folder
        std::filesystem::path createModelFiles(const Node& node, const std::string& modelName, const std::string& parentFolder);

        // Creates a single .tesmesh file from mesh data
        void createSingleMeshFile(const aiMeshData& meshData, const std::string& outputPath);

        AssimpLoader::ModelData loadModel(const std::string& path);

        // Loads a .temodel file
        TEModelData loadTEModel(const std::string& path);

    private:
        ModelData processScene(const std::string& modelName, aiNode* aiNode, const aiScene* aiScene);

        aiMeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);

        // Helper to convert Node to ModelNode while creating mesh files
        ModelNode convertNodeToModelNode(const Node& node, const std::string& modelName, const std::string& outputFolder, const std::string& nodePath = "");
    };
}
