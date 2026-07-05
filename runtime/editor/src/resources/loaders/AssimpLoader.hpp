#pragma once

#include "../../fileSystem/FileSystem.hpp" // For some reason it cannot find this file normally
#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "TechEngine/core/resources/material/MaterialResource.hpp"
#include "resources/ResourceSystem.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include <glm/glm.hpp>

namespace TechEngine {
    class AssimpLoader {
    public:
        struct aiMeshData {
            std::string name;
            std::vector<Vertex> vertices; // Vertices of the mesh from Vertex.hpp Might change to internal format
            std::vector<int> indices;
            std::string material; // Material name

            static void serialize(StreamWriter* writer, const aiMeshData& meshData) {
                writer->writeArray(meshData.vertices);
                writer->writeArray(meshData.indices);
            }

            static void deserialize(StreamReader* reader, aiMeshData& meshData) {
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

        struct SceneData {
            Node rootNode;
            std::vector<std::string> materialNames;
            std::unordered_map<std::string, std::shared_ptr<MaterialResource>> materials; // Material name -> MaterialData
            std::unordered_map<std::string, std::shared_ptr<TextureResource>> textures; // Texture name -> TextureData
        };


        ResourceSystem& m_resourceSystem;
        FileSystem& m_fileSystem;
        TextureLoader& m_textureLoader;

    public:
        AssimpLoader(ResourceSystem& m_resourcesSystem, FileSystem& fileSystem, TextureLoader& textureLoader);

        SceneData loadSourceFile(const std::filesystem::path& virtualPath);

    private:
        SceneData processScene(const std::string& modelName, aiNode* aiNode, const aiScene* aiScene);

        aiMeshData processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene);

        std::shared_ptr<TextureResource> processEmbeddedTexture(const aiTexture* texture, const std::string& modelDirectory);

        std::shared_ptr<TextureResource> processExternalTexture(const std::filesystem::path& filePath, const std::filesystem::path& modelDirectory, const aiMaterial* material) const;

        std::tuple<std::shared_ptr<MaterialResource>, std::vector<std::shared_ptr<TextureResource>>> processMaterialandItsTextures(aiMaterial* material, const std::filesystem::path& modelDirectory, std::unordered_map<std::string, std::shared_ptr<TextureResource>>& textureAccumulator);
    };
}
