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
        // Material data extracted from Assimp
        struct MaterialData {
            std::string name;
            glm::vec4 albedoColor = glm::vec4(1.0f);
            float metallic = 0.0f;
            float roughness = 0.5f;

            // Texture paths (relative to model file)
            std::string albedoTexture;
            std::string normalTexture;
            std::string metallicTexture;
            std::string roughnessTexture;
            std::string aoTexture;
            std::string emissionTexture;

            static void Serialize(StreamWriter* writer, const MaterialData& mat) {
                writer->writeString(mat.name);
                writer->writeRaw(mat.albedoColor);
                writer->writeRaw(mat.metallic);
                writer->writeRaw(mat.roughness);
                writer->writeString(mat.albedoTexture);
                writer->writeString(mat.normalTexture);
                writer->writeString(mat.metallicTexture);
                writer->writeString(mat.roughnessTexture);
                writer->writeString(mat.aoTexture);
                writer->writeString(mat.emissionTexture);
            }

            static void Deserialize(StreamReader* reader, MaterialData& mat) {
                reader->readString(mat.name);
                reader->readRaw(mat.albedoColor);
                reader->readRaw(mat.metallic);
                reader->readRaw(mat.roughness);
                reader->readString(mat.albedoTexture);
                reader->readString(mat.normalTexture);
                reader->readString(mat.metallicTexture);
                reader->readString(mat.roughnessTexture);
                reader->readString(mat.aoTexture);
                reader->readString(mat.emissionTexture);
            }
        };

        struct aiMeshData {
            std::vector<Vertex> vertices; // Vertices of the mesh from Vertex.hpp Might change to internal format
            std::vector<int> indices;
            std::string material; // Material name

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
            std::vector<std::string> materialNames;
            std::unordered_map<std::string, MaterialData> materials; // Material name -> MaterialData
        };

        // Serializable node for .temodel file (references mesh files instead of containing mesh data)
        struct ModelNode {
            std::string name;
            glm::vec3 position = glm::vec3(0.0f);
            glm::vec3 rotation = glm::vec3(0.0f);
            glm::vec3 scale = glm::vec3(1.0f);
            std::vector<std::string> meshFiles;     // Relative paths to .tesmesh files
            std::vector<std::string> materialNames; // Material name for each mesh (parallel to meshFiles)
            std::vector<ModelNode> children;

            static void Serialize(StreamWriter* writer, const ModelNode& node) {
                writer->writeString(node.name);
                writer->writeRaw(node.position);
                writer->writeRaw(node.rotation);
                writer->writeRaw(node.scale);
                writer->writeArray(node.meshFiles);
                writer->writeArray(node.materialNames);
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
                reader->readArray(node.materialNames);
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
            std::vector<MaterialData> materials; // All materials used by this model

            static void Serialize(StreamWriter* writer, const TEModelData& data) {
                writer->writeString(data.modelName);
                ModelNode::Serialize(writer, data.rootNode);
                writer->writeRaw<uint32_t>(static_cast<uint32_t>(data.materials.size()));
                for (const auto& mat : data.materials) {
                    MaterialData::Serialize(writer, mat);
                }
            }

            static void Deserialize(StreamReader* reader, TEModelData& data) {
                reader->readString(data.modelName);
                ModelNode::Deserialize(reader, data.rootNode);
                uint32_t matCount = 0;
                reader->readRaw<uint32_t>(matCount);
                data.materials.resize(matCount);
                for (uint32_t i = 0; i < matCount; ++i) {
                    MaterialData::Deserialize(reader, data.materials[i]);
                }
            }
        };

    public:
        // Creates a single merged .tesmesh file from all meshes in the hierarchy
        std::filesystem::path createStaticMeshFile(const Node& node, const std::string& staticMeshPath);

        // Creates a .temodel file and individual .tesmesh files in a folder
        std::filesystem::path createModelFiles(const ModelData& modelData, const std::string& modelName, const std::string& parentFolder);

        // Creates a single .tesmesh file from mesh data
        void createSingleMeshFile(const aiMeshData& meshData, const std::string& outputPath);

        ModelData loadModel(const std::string& path);

        // Loads a .temodel file
        TEModelData loadTEModel(const std::string& path);

    private:
        ModelData processScene(const std::string& modelName, aiNode* aiNode, const aiScene* aiScene);

        aiMeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);

        MaterialData processMaterial(aiMaterial* material, const std::string& modelDirectory);

        // Creates a .mat file from MaterialData
        void createMaterialFile(const MaterialData& matData, const std::string& outputFolder);

        // Helper to convert Node to ModelNode while creating mesh files
        ModelNode convertNodeToModelNode(const Node& node, const ModelData& modelData, const std::string& modelName, const std::string& outputFolder, const std::string& nodePath = "");
    };
}
