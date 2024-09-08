#include "AssimpLoader.hpp"


#include "core/Logger.hpp"
#include "resources/material/MaterialManager.hpp"
#include "files/FileUtils.hpp"
#include "Mesh.hpp"

#include "assimp/Importer.hpp"
#include <filesystem>
#include <fstream>
#include <queue>
#include <assimp/postprocess.h>
#include <yaml-cpp/yaml.h>

namespace TechEngine {
    std::filesystem::path AssimpLoader::createStaticMeshFile(const AssimpLoader::Node& node, const std::string& staticMeshPath) {
        std::queue<AssimpLoader::Node> nodeQueue;
        nodeQueue.push(node);

        int lastIndex = 0;
        FileStreamWriter writer(staticMeshPath);
        while (!nodeQueue.empty()) {
            // Get the current node
            AssimpLoader::Node currentNode = nodeQueue.front();
            nodeQueue.pop();

            // Add current node's meshes to the list
            for (AssimpLoader::aiMeshData& meshData: currentNode.meshes) {
                writer.writeArray(meshData.vertices);

                std::vector<int> indices;
                for (int index: meshData.indices) {
                    indices.push_back(index + lastIndex);
                }
                writer.writeArray(meshData.indices);
                lastIndex = meshData.indices.back();
            }

            // Add current node's children to the queue
            for (const AssimpLoader::Node& child: currentNode.children) {
                nodeQueue.push(child);
            }
        }

        return staticMeshPath;
    }

    AssimpLoader::ModelData AssimpLoader::loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* aiScene = importer.ReadFile(path, aiProcess_Triangulate);

        if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode) {
            TE_LOGGER_WARN("Assimp error: {}", importer.GetErrorString());
        }

        // Process the scene and create a game object
        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials and {2} textures", aiScene->mNumMeshes, aiScene->mNumMaterials, aiScene->mNumTextures);
        AssimpLoader::ModelData modelData = processScene(FileUtils::getFileName(path), aiScene->mRootNode, aiScene);
        return modelData;
    }

    AssimpLoader::ModelData AssimpLoader::processScene(const std::string& modelName, aiNode* ai_node, const aiScene* aiScene) {
        ModelData modelData;
        Node node;
        std::queue<std::pair<Node*, aiNode*>> nodeQueue;
        nodeQueue.emplace(&node, ai_node);
        while (!nodeQueue.empty()) {
            Node* currentNode = nodeQueue.front().first;
            aiNode* currentAINode = nodeQueue.front().second;
            nodeQueue.pop();

            for (unsigned int i = 0; i < currentAINode->mNumMeshes; i++) {
                aiVector3t<float> scaling;
                aiVector3t<float> rotation;
                aiVector3t<float> position;
                currentAINode->mTransformation.Decompose(scaling, rotation, position);
                aiMesh* aiMesh = aiScene->mMeshes[currentAINode->mMeshes[i]];

                aiMeshData processedMesh = processMesh(modelName, aiMesh, aiScene);
                currentNode->meshes.emplace_back(processedMesh);
                modelData.materials.push_back(processedMesh.material);
            }
            for (unsigned int i = 0; i < currentAINode->mNumChildren; i++) {
                Node childNode;
                currentNode->children.emplace_back(childNode);
                nodeQueue.push({&currentNode->children.back(), currentAINode->mChildren[i]});
            }
        }
        modelData.rootNode = node;

        return modelData;
    }

    AssimpLoader::aiMeshData AssimpLoader::processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene) {
        aiMeshData meshData;
        for (int i = 0; i < mesh->mNumVertices; i++) {
            glm::vec3 position;
            position.x = mesh->mVertices[i].x;
            position.y = mesh->mVertices[i].y;
            position.z = mesh->mVertices[i].z;

            glm::vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;

            glm::vec2 textureCoordinate;
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                textureCoordinate.x = mesh->mTextureCoords[0][i].x;
                textureCoordinate.y = mesh->mTextureCoords[0][i].y;
            } else
                textureCoordinate = glm::vec2(0.0f, 0.0f);
            meshData.vertices.emplace_back(position, normal, textureCoordinate);
        }

        for (int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (int j = 0; j < face.mNumIndices; j++)
                meshData.indices.push_back(face.mIndices[j]);
        }
        meshData.material = scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
        TE_LOGGER_INFO("Material: {}", meshData.material);
        return meshData;
    }
}
