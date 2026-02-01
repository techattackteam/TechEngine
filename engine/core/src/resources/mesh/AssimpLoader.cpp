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
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

namespace TechEngine {
    // Helper function to compute the transformation matrix from node properties
    static glm::mat4 computeNodeTransformMatrix(const AssimpLoader::Node& node) {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), node.position);
        glm::mat4 rotation = glm::toMat4(glm::quat(glm::radians(node.rotation)));
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), node.scale);
        return translation * rotation * scale;
    }

    // Helper function to transform vertices by a matrix
    static std::vector<Vertex> transformVertices(const std::vector<Vertex>& vertices, const glm::mat4& transform) {
        std::vector<Vertex> transformedVertices;
        transformedVertices.reserve(vertices.size());

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

        for (const Vertex& v : vertices) {
            Vertex transformed;
            // Transform position
            glm::vec4 pos = transform * glm::vec4(v.position, 1.0f);
            transformed.position = glm::vec3(pos);
            // Transform normal (using normal matrix to handle non-uniform scaling)
            transformed.normal = glm::normalize(normalMatrix * v.normal);
            // UV coordinates don't need transformation
            transformed.texCoords = v.texCoords;
            transformedVertices.push_back(transformed);
        }
        return transformedVertices;
    }

    // Recursive helper to collect all meshes with their world transforms
    static void collectMeshesWithTransforms(const AssimpLoader::Node& node, const glm::mat4& parentTransform,
                                            std::vector<Vertex>& allVertices, std::vector<int>& allIndices) {
        // Compute world transform for this node
        glm::mat4 localTransform = computeNodeTransformMatrix(node);
        glm::mat4 worldTransform = parentTransform * localTransform;

        // Process all meshes in this node
        for (const AssimpLoader::aiMeshData& meshData : node.meshes) {
            // Transform vertices to world space
            std::vector<Vertex> transformedVertices = transformVertices(meshData.vertices, worldTransform);

            // Add indices with offset
            int indexOffset = static_cast<int>(allVertices.size());
            for (int index : meshData.indices) {
                allIndices.push_back(index + indexOffset);
            }

            // Add transformed vertices
            allVertices.insert(allVertices.end(), transformedVertices.begin(), transformedVertices.end());
        }

        // Recursively process children
        for (const AssimpLoader::Node& child : node.children) {
            collectMeshesWithTransforms(child, worldTransform, allVertices, allIndices);
        }
    }

    std::filesystem::path AssimpLoader::createStaticMeshFile(const AssimpLoader::Node& node, const std::string& staticMeshPath) {
        // Collect all vertices and indices with transforms applied
        std::vector<Vertex> allVertices;
        std::vector<int> allIndices;

        collectMeshesWithTransforms(node, glm::mat4(1.0f), allVertices, allIndices);

        // Write to file
        FileStreamWriter writer(staticMeshPath);
        writer.writeArray(allVertices);
        writer.writeArray(allIndices);

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
