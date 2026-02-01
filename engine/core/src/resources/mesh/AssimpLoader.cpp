#include "AssimpLoader.hpp"


#include "core/Logger.hpp"
#include "TechEngine/core/resources/material/MaterialManager.hpp"
#include "files/FileUtils.hpp"

#include "assimp/Importer.hpp"
#include <filesystem>
#include <fstream>
#include <queue>
#include <assimp/postprocess.h>
#include <yaml-cpp/yaml.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

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

    void AssimpLoader::createSingleMeshFile(const aiMeshData& meshData, const std::string& outputPath) {
        FileStreamWriter writer(outputPath);
        writer.writeArray(meshData.vertices);
        writer.writeArray(meshData.indices);
    }

    std::filesystem::path AssimpLoader::createModelFiles(const Node& node, const std::string& modelName, const std::string& parentFolder) {
        std::string modelFolder = parentFolder + "\\" + modelName;
        std::filesystem::path folderPath = parentFolder;
        if (!std::filesystem::exists(folderPath)) {
            std::filesystem::create_directories(folderPath);
        }
        std::string meshesFolder = modelFolder + "\\meshes";
        if (!std::filesystem::exists(meshesFolder)) {
            std::filesystem::create_directories(meshesFolder);
        }
        ModelNode modelNode = convertNodeToModelNode(node, modelName, meshesFolder);

        // Create the .temodel file
        TEModelData teModelData;
        teModelData.modelName = modelName;
        teModelData.rootNode = modelNode;

        std::string teModelPath = parentFolder + "\\" + modelName + ".temodel";
        FileStreamWriter writer(teModelPath);
        TEModelData::Serialize(&writer, teModelData);

        TE_LOGGER_INFO("Created model file: {}", teModelPath);
        return teModelPath;
    }

    AssimpLoader::ModelNode AssimpLoader::convertNodeToModelNode(const Node& node, const std::string& modelName, const std::string& outputFolder, const std::string& nodePath) {
        ModelNode modelNode;
        modelNode.name = node.name;
        modelNode.position = node.position;
        modelNode.rotation = node.rotation;
        modelNode.scale = node.scale;

        std::string currentNodePath = nodePath.empty() ? node.name : nodePath + "_" + node.name;
        if (currentNodePath.empty()) {
            currentNodePath = modelName;
        }

        for (size_t i = 0; i < node.meshes.size(); ++i) {
            std::string meshFileName = modelName + "_" + currentNodePath + "_mesh" + std::to_string(i);
            std::replace(meshFileName.begin(), meshFileName.end(), '/', '_');
            std::replace(meshFileName.begin(), meshFileName.end(), '\\', '_');

            std::string meshFilePath = outputFolder + "\\" + meshFileName + ".tesmesh";
            createSingleMeshFile(node.meshes[i], meshFilePath);

            modelNode.meshFiles.push_back(meshFileName + ".tesmesh");
            TE_LOGGER_INFO("Created mesh file: {}", meshFilePath);
        }

        for (const Node& child: node.children) {
            modelNode.children.push_back(convertNodeToModelNode(child, modelName, outputFolder, currentNodePath));
        }

        return modelNode;
    }

    AssimpLoader::TEModelData AssimpLoader::loadTEModel(const std::string& path) {
        FileStreamReader reader(path);
        TEModelData data;
        TEModelData::Deserialize(&reader, data);
        return data;
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

            currentNode->name = currentAINode->mName.C_Str();

            aiVector3t<float> scaling;
            aiVector3t<float> rotation;
            aiVector3t<float> position;
            currentAINode->mTransformation.Decompose(scaling, rotation, position);

            currentNode->position = glm::vec3(position.x, position.y, position.z);
            currentNode->rotation = glm::degrees(glm::vec3(rotation.x, rotation.y, rotation.z));
            currentNode->scale = glm::vec3(scaling.x, scaling.y, scaling.z);

            for (unsigned int i = 0; i < currentAINode->mNumMeshes; i++) {
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
