#include "AssimpLoader.hpp"


#include "core/Logger.hpp"
#include "TechEngine/core/resources/material/MaterialManager.hpp"
#include "files/FileUtils.hpp"

#include "assimp/Importer.hpp"
#include <assimp/material.h>
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

        for (const Vertex& v: vertices) {
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
        for (const AssimpLoader::aiMeshData& meshData: node.meshes) {
            // Transform vertices to world space
            std::vector<Vertex> transformedVertices = transformVertices(meshData.vertices, worldTransform);

            // Add indices with offset
            int indexOffset = static_cast<int>(allVertices.size());
            for (int index: meshData.indices) {
                allIndices.push_back(index + indexOffset);
            }

            // Add transformed vertices
            allVertices.insert(allVertices.end(), transformedVertices.begin(), transformedVertices.end());
        }

        // Recursively process children
        for (const AssimpLoader::Node& child: node.children) {
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

    void AssimpLoader::createMaterialFile(const MaterialData& matData, const std::string& outputFolder) {
        std::string filepath = outputFolder + "\\" + matData.name + ".mat";

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << matData.name;
        out << YAML::Key << "albedo" << YAML::Value << YAML::Flow << YAML::BeginSeq
                << matData.albedoColor.x << matData.albedoColor.y << matData.albedoColor.z << matData.albedoColor.w
                << YAML::EndSeq;
        out << YAML::Key << "metallic" << YAML::Value << matData.metallic;
        out << YAML::Key << "roughness" << YAML::Value << matData.roughness;
        out << YAML::Key << "ambientOcclusion" << YAML::Value << 1.0f;
        out << YAML::Key << "emission" << YAML::Value << YAML::Flow << YAML::BeginSeq
                << 0.0f << 0.0f << 0.0f << 0.0f
                << YAML::EndSeq;

        // Add texture paths if available
        if (!matData.albedoTexture.empty()) {
            out << YAML::Key << "albedoTexture" << YAML::Value << matData.albedoTexture;
        }
        if (!matData.normalTexture.empty()) {
            out << YAML::Key << "normalTexture" << YAML::Value << matData.normalTexture;
        }
        if (!matData.metallicTexture.empty()) {
            out << YAML::Key << "metallicTexture" << YAML::Value << matData.metallicTexture;
        }
        if (!matData.roughnessTexture.empty()) {
            out << YAML::Key << "roughnessTexture" << YAML::Value << matData.roughnessTexture;
        }
        if (!matData.aoTexture.empty()) {
            out << YAML::Key << "aoTexture" << YAML::Value << matData.aoTexture;
        }
        if (!matData.emissionTexture.empty()) {
            out << YAML::Key << "emissionTexture" << YAML::Value << matData.emissionTexture;
        }

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
        fout.close();

        TE_LOGGER_INFO("Created material file: {}", filepath);
    }

    std::filesystem::path AssimpLoader::createModelFiles(const ModelData& modelData, const std::string& modelName, const std::string& parentFolder) {
        std::string modelFolder = parentFolder + "\\" + modelName;
        std::filesystem::path folderPath = parentFolder;
        if (!std::filesystem::exists(folderPath)) {
            std::filesystem::create_directories(folderPath);
        }
        std::string meshesFolder = modelFolder + "\\meshes";
        if (!std::filesystem::exists(meshesFolder)) {
            std::filesystem::create_directories(meshesFolder);
        }

        // Create materials folder and .mat files
        std::string materialsFolder = modelFolder + "\\materials";
        if (!std::filesystem::exists(materialsFolder)) {
            std::filesystem::create_directories(materialsFolder);
        }

        // Create textures folder
        std::string texturesFolder = modelFolder + "\\textures";
        if (!std::filesystem::exists(texturesFolder)) {
            std::filesystem::create_directories(texturesFolder);
        }

        // Helper lambda to copy texture and return new relative path
        auto copyTextureIfExists = [&](const std::string& texturePath) -> std::string {
            if (texturePath.empty() || !std::filesystem::exists(texturePath)) {
                return "";
            }
            std::string filename = std::filesystem::path(texturePath).filename().string();
            std::string destPath = texturesFolder + "\\" + filename;

            // Only copy if destination doesn't exist
            if (!std::filesystem::exists(destPath)) {
                try {
                    std::filesystem::copy_file(texturePath, destPath, std::filesystem::copy_options::overwrite_existing);
                    TE_LOGGER_INFO("Copied texture: {} -> {}", texturePath, destPath);
                } catch (const std::exception& e) {
                    TE_LOGGER_WARN("Failed to copy texture {}: {}", texturePath, e.what());
                    return texturePath; // Return original path if copy fails
                }
            }
            return destPath;
        };

        // Create .mat files for each material with updated texture paths
        std::vector<MaterialData> updatedMaterials;
        for (const auto& [name, matData]: modelData.materials) {
            // Create a copy of the material data with updated texture paths
            MaterialData updatedMatData = matData;
            updatedMatData.albedoTexture = copyTextureIfExists(matData.albedoTexture);
            updatedMatData.normalTexture = copyTextureIfExists(matData.normalTexture);
            updatedMatData.metallicTexture = copyTextureIfExists(matData.metallicTexture);
            updatedMatData.roughnessTexture = copyTextureIfExists(matData.roughnessTexture);
            updatedMatData.aoTexture = copyTextureIfExists(matData.aoTexture);
            updatedMatData.emissionTexture = copyTextureIfExists(matData.emissionTexture);

            createMaterialFile(updatedMatData, materialsFolder);
            updatedMaterials.push_back(updatedMatData);
        }

        ModelNode modelNode = convertNodeToModelNode(modelData.rootNode, modelData, modelName, meshesFolder);

        // Create the .temodel file
        TEModelData teModelData;
        teModelData.modelName = modelName;
        teModelData.rootNode = modelNode;

        // Add all materials with updated texture paths to the temodel
        teModelData.materials = updatedMaterials;

        std::string teModelPath = parentFolder + "\\" + modelName + ".temodel";
        FileStreamWriter writer(teModelPath);
        TEModelData::Serialize(&writer, teModelData);

        TE_LOGGER_INFO("Created model file: {} with {} materials", teModelPath, teModelData.materials.size());
        return teModelPath;
    }

    AssimpLoader::ModelNode AssimpLoader::convertNodeToModelNode(const Node& node, const ModelData& modelData, const std::string& modelName, const std::string& outputFolder, const std::string& nodePath) {
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
            modelNode.materialNames.push_back(node.meshes[i].material);
            TE_LOGGER_INFO("Created mesh file: {} with material: {}", meshFilePath, node.meshes[i].material);
        }

        for (const Node& child: node.children) {
            modelNode.children.push_back(convertNodeToModelNode(child, modelData, modelName, outputFolder, currentNodePath));
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

        // Get the directory containing the model file (for resolving texture paths)
        std::string modelDirectory = std::filesystem::path(path).parent_path().string();

        // Process the scene and create a game object
        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials and {2} textures", aiScene->mNumMeshes, aiScene->mNumMaterials, aiScene->mNumTextures);
        AssimpLoader::ModelData modelData = processScene(FileUtils::getFileName(path), aiScene->mRootNode, aiScene);

        // Process all materials from the scene
        for (unsigned int i = 0; i < aiScene->mNumMaterials; ++i) {
            MaterialData matData = processMaterial(aiScene->mMaterials[i], modelDirectory);
            modelData.materials[matData.name] = matData;
        }

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
                modelData.materialNames.push_back(processedMesh.material);
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

    AssimpLoader::MaterialData AssimpLoader::processMaterial(aiMaterial* material, const std::string& modelDirectory) {
        MaterialData matData;

        // Get material name
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            matData.name = name.C_Str();
        } else {
            matData.name = "Material_" + std::to_string(reinterpret_cast<uintptr_t>(material));
        }

        // Get base color/albedo
        aiColor4D color;
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            matData.albedoColor = glm::vec4(color.r, color.g, color.b, color.a);
        }

        // Get metallic factor
        float metallic;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
            matData.metallic = metallic;
        }

        // Get roughness factor
        float roughness;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
            matData.roughness = roughness;
        }

        // Helper lambda to get texture path
        auto getTexturePath = [&](aiTextureType type) -> std::string {
            if (material->GetTextureCount(type) > 0) {
                aiString texPath;
                if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
                    std::string path = texPath.C_Str();
                    // If path is relative, make it relative to model directory
                    if (!path.empty() && path[0] != '/' && path[0] != '\\' && (path.size() < 2 || path[1] != ':')) {
                        return modelDirectory + "\\" + path;
                    }
                    return path;
                }
            }
            return "";
        };

        // Get texture paths
        matData.albedoTexture = getTexturePath(aiTextureType_DIFFUSE);
        if (matData.albedoTexture.empty()) {
            matData.albedoTexture = getTexturePath(aiTextureType_BASE_COLOR);
        }
        matData.normalTexture = getTexturePath(aiTextureType_NORMALS);
        if (matData.normalTexture.empty()) {
            matData.normalTexture = getTexturePath(aiTextureType_HEIGHT);
        }
        matData.metallicTexture = getTexturePath(aiTextureType_METALNESS);
        matData.roughnessTexture = getTexturePath(aiTextureType_DIFFUSE_ROUGHNESS);
        if (matData.roughnessTexture.empty()) {
            matData.roughnessTexture = getTexturePath(aiTextureType_SHININESS);
        }
        matData.aoTexture = getTexturePath(aiTextureType_AMBIENT_OCCLUSION);
        if (matData.aoTexture.empty()) {
            matData.aoTexture = getTexturePath(aiTextureType_LIGHTMAP);
        }
        matData.emissionTexture = getTexturePath(aiTextureType_EMISSIVE);

        TE_LOGGER_INFO("Processed material '{}': albedo={}, normal={}, metallic={}, roughness={}",
                       matData.name,
                       matData.albedoTexture.empty() ? "none" : matData.albedoTexture,
                       matData.normalTexture.empty() ? "none" : matData.normalTexture,
                       matData.metallicTexture.empty() ? "none" : matData.metallicTexture,
                       matData.roughnessTexture.empty() ? "none" : matData.roughnessTexture);

        return matData;
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
        aiString materialName;
        if (scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
            meshData.material = materialName.C_Str();
        } else {
            meshData.material = "Material_" + std::to_string(reinterpret_cast<uintptr_t>(scene->mMaterials[mesh->mMaterialIndex]));
        }

        TE_LOGGER_INFO("Material: {}", meshData.material);
        return meshData;
    }
}
