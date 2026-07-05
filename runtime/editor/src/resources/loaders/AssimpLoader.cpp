#include "AssimpLoader.hpp"

#include "core/Logger.hpp"
#include "files/FileUtils.hpp"

#include "assimp/Importer.hpp"
#include <assimp/material.h>
#include <filesystem>
#include <fstream>
#include <queue>
#include <assimp/postprocess.h>
#include <yaml-cpp/yaml.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>

#include "TextureLoader.hpp"

namespace TechEngine {
    AssimpLoader::AssimpLoader(ResourceSystem& resourceSystem,
                               FileSystem& fileSystem,
                               TextureLoader& textureLoader) : m_resourceSystem(resourceSystem),
                                                               m_fileSystem(fileSystem),
                                                               m_textureLoader(textureLoader) {
    }


    AssimpLoader::SceneData AssimpLoader::loadSourceFile(const std::filesystem::path& virtualPath) {
        Assimp::Importer importer;
        std::filesystem::path physicalPath = m_fileSystem.resolve(virtualPath);
        const aiScene* aiScene = importer.ReadFile(physicalPath.string(), aiProcess_Triangulate |
                                                                          aiProcess_CalcTangentSpace |
                                                                          aiProcess_GenSmoothNormals |
                                                                          aiProcess_JoinIdenticalVertices);

        if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode) {
            TE_LOGGER_WARN("Assimp error: {0}", importer.GetErrorString());
            return {};
        }

        std::string modelDirectory = m_fileSystem.getParentPath(virtualPath).string();
        std::string fileName = m_fileSystem.status(virtualPath).name.string();
        SceneData modelData = processScene(fileName, aiScene->mRootNode, aiScene);

        for (unsigned int i = 0; i < aiScene->mNumTextures; i++) {
            std::shared_ptr<TextureResource> textureData = processEmbeddedTexture(aiScene->mTextures[i], modelDirectory);
            if (textureData) {
                modelData.textures[textureData->getName()] = textureData;
            }
        }

        std::unordered_set<unsigned int> usedMaterialIndices;
        for (unsigned int i = 0; i < aiScene->mNumMeshes; i++) {
            usedMaterialIndices.insert(aiScene->mMeshes[i]->mMaterialIndex);
        }

        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials (out of {2}) and {3} textures",
                       aiScene->mNumMeshes, usedMaterialIndices.size(), aiScene->mNumMaterials, aiScene->mNumTextures);

        std::unordered_map<std::string, std::shared_ptr<TextureResource>> textureAccumulator;
        for (unsigned int matIndex: usedMaterialIndices) {
            if (matIndex >= aiScene->mNumMaterials) {
                continue;
            }

            auto matData = processMaterialandItsTextures(aiScene->mMaterials[matIndex], modelDirectory, textureAccumulator);
            std::shared_ptr<MaterialResource> material = std::get<0>(matData);
            std::vector<std::shared_ptr<TextureResource>> textures = std::get<1>(matData);

            modelData.materials[material->getName()] = material;
            /*for (const auto& texture: textures) {
                if (texture) {
                    modelData.textures[texture->getName()] = texture;
                }
            }*/
        }
        for (const auto& [name, texture]: textureAccumulator) {
            modelData.textures[name] = texture;
        }
        return modelData;
    }

    AssimpLoader::SceneData AssimpLoader::processScene(const std::string& modelName, aiNode* ai_node, const aiScene* aiScene) {
        SceneData modelData;
        Node node;
        std::queue<std::pair<Node*, aiNode*>> nodeQueue;
        nodeQueue.emplace(&node, ai_node);
        while (!nodeQueue.empty()) {
            Node* currentNode = nodeQueue.front().first;
            aiNode* currentAINode = nodeQueue.front().second;
            nodeQueue.pop();

            currentNode->name = currentAINode->mName.C_Str();

            aiVector3t<float> scaling;
            aiVector3t<float> position;
            aiQuaternion rotation;
            currentAINode->mTransformation.Decompose(scaling, rotation, position);
            glm::quat q(rotation.w, rotation.x, rotation.y, rotation.z);
            currentNode->rotation = glm::degrees(glm::eulerAngles(q));
            currentNode->position = glm::vec3(position.x, position.y, position.z);
            currentNode->scale = glm::vec3(scaling.x, scaling.y, scaling.z);

            for (unsigned int i = 0; i < currentAINode->mNumMeshes; i++) {
                aiMesh* aiMesh = aiScene->mMeshes[currentAINode->mMeshes[i]];

                aiMeshData processedMesh = processMesh(modelName, aiMesh, aiScene);
                currentNode->meshes.push_back(processedMesh);
            }
            currentNode->children.reserve(currentAINode->mNumChildren);
            for (unsigned int i = 0; i < currentAINode->mNumChildren; i++) {
                Node childNode;
                currentNode->children.push_back(childNode);
                nodeQueue.emplace(&currentNode->children.back(), currentAINode->mChildren[i]);
            }
        }
        modelData.rootNode = node;

        return modelData;
    }

    AssimpLoader::aiMeshData AssimpLoader::processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene) {
        aiMeshData meshData;
        meshData.name = mesh->mName.C_Str();
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

        return meshData;
    }

    std::shared_ptr<TextureResource> AssimpLoader::processEmbeddedTexture(const aiTexture* texture, const std::string& modelDirectory) {
        std::shared_ptr<TextureResource> textureResource;
        int w, h, ch;
        stbi_uc* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(texture->pcData),
                                              texture->mWidth, &w, &h, &ch, 0);
        //m_textureLoader.loadTextureFromMemory(data, w, h, ch, texture->mFilename.C_Str());
        return nullptr;
    }

    std::shared_ptr<TextureResource> AssimpLoader::processExternalTexture(const std::filesystem::path& filePath, const std::filesystem::path& modelDirectory, const aiMaterial* material) const {
        std::filesystem::path fileName = filePath.filename();
        std::filesystem::path physicalPath = modelDirectory / std::filesystem::path(fileName.string());

        std::shared_ptr<TextureResource> texture = m_textureLoader.loadTextureFromImageFile(physicalPath.string(), fileName.stem().string());
        aiTextureMapMode mapModeU;
        aiTextureMapMode mapModeV;
        material->Get(AI_MATKEY_MAPPINGMODE_U(aiTextureType_BASE_COLOR, 0), mapModeU);
        material->Get(AI_MATKEY_MAPPINGMODE_V(aiTextureType_BASE_COLOR, 0), mapModeV);

        TextureMode modeU;
        TextureMode modeV;

        switch (mapModeU) {
            case aiTextureMapMode_Wrap:
                modeU = REPEAT;
                break;
            case aiTextureMapMode_Clamp:
                modeU = CLAMP_TO_EDGE;
                break;
            case aiTextureMapMode_Mirror:
                modeU = MIRRORED_REPEAT;
                break;
            case aiTextureMapMode_Decal:
                // Decal usually means pixels outside the range are transparent/border color
                modeU = CLAMP_TO_BORDER;
                break;
            default:
                modeU = REPEAT; // Safe default
                break;
        }
        switch (mapModeV) {
            case aiTextureMapMode_Wrap:
                modeV = REPEAT;
                break;
            case aiTextureMapMode_Clamp:
                modeV = CLAMP_TO_EDGE;
                break;
            case aiTextureMapMode_Mirror:
                modeV = MIRRORED_REPEAT;
                break;
            case aiTextureMapMode_Decal:
                // Decal usually means pixels outside the range are transparent/border color
                modeV = CLAMP_TO_BORDER;
                break;
            default:
                modeV = REPEAT; // Safe default
                break;
        }

        texture->setMapMode(modeU, modeV);
        return texture;
    }

    std::tuple<std::shared_ptr<MaterialResource>, std::vector<std::shared_ptr<TextureResource>>>
    AssimpLoader::processMaterialandItsTextures(
        aiMaterial* material,
        const std::filesystem::path& modelDirectory,
        std::unordered_map<std::string, std::shared_ptr<TextureResource>>& textureAccumulator) {
        std::string materialName;
        // Get material name
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            materialName = name.C_Str();
        } else {
            materialName = "Material_" + std::to_string(reinterpret_cast<uintptr_t>(material));
        }
        std::shared_ptr<MaterialResource> materialResource = std::make_shared<MaterialResource>(materialName);

        std::filesystem::path fileName = material->GetName().C_Str();
        std::filesystem::path extension = std::filesystem::path(fileName).extension().string();
        std::filesystem::path physicalPath = modelDirectory / std::filesystem::path(fileName.string() + "." + extension.string());

        std::vector<std::shared_ptr<TextureResource>> textures;

        // Get base color/albedo
        aiColor4D color;
        glm::vec4 albedoColor(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            albedoColor = glm::vec4(color.r, color.g, color.b, color.a);
            materialResource->setAlbedo(albedoColor);
        }

        // Get metallic factor
        float metallic;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
            materialResource->setMetallic(metallic);
        }

        // Get roughness factor
        float roughness;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
            materialResource->setRoughness(roughness);
        }


        // Helper lambda to get texture path
        auto getTextureUUID = [&](aiTextureType type) -> UUID {
            if (material->GetTextureCount(type) > 0) {
                aiString texturePath;
                if (material->GetTexture(type, 0, &texturePath) == AI_SUCCESS) {
                    std::string pathStr = texturePath.C_Str();
                    std::string stemName = std::filesystem::path(pathStr).stem().string();

                    if (m_resourceSystem.isTextureRegistered(stemName)) {
                        return m_resourceSystem.getTextureResource(stemName)->getUUID();
                    }

                    auto it = textureAccumulator.find(stemName);
                    if (it != textureAccumulator.end()) {
                        return it->second->getUUID(); // reuse same object, same UUID
                    }

                    std::shared_ptr<TextureResource> texture = processExternalTexture(std::filesystem::path(pathStr), modelDirectory, material);
                    textureAccumulator[stemName] = texture;
                    return texture->getUUID();
                }
            }
            return UUID(-1);
        };

        //Get texture paths
        if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0) {
            materialResource->setAlbedoMapID(getTextureUUID(aiTextureType_BASE_COLOR));
        }

        if (material->GetTextureCount(aiTextureType_NORMALS) > 0) {
            materialResource->setNormalMapID(getTextureUUID(aiTextureType_NORMALS));
        }

        if (material->GetTextureCount(aiTextureType_METALNESS) > 0) {
            materialResource->setMetallicMapID(getTextureUUID(aiTextureType_METALNESS));
        }

        if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0) {
            materialResource->setRoughnessMapID(getTextureUUID(aiTextureType_DIFFUSE_ROUGHNESS));
        }
        /*if (material->GetTextureCount(aiTextureType_SHININESS) > 0 && materialResource->getRoughnessMapUUID() == -1) {
            materialResource->setRoughnessMapID(getTextureUUID(aiTextureType_SHININESS));
        }*/
        if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0) {
            materialResource->setAmbientOcclusionMapID(getTextureUUID(aiTextureType_AMBIENT_OCCLUSION));
        }
        /*if (material->GetTextureCount(aiTextureType_LIGHTMAP) > 0 && materialResource->getAmbientOcclusionMapUUID() == -1) {
            materialResource->setAmbientOcclusionMapID(getTextureUUID(aiTextureType_LIGHTMAP));
        }*/
        if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0) {
            materialResource->setEmissionMapID(getTextureUUID(aiTextureType_EMISSIVE));
        }

        TE_LOGGER_INFO("Processed material '{0}': albedo={1}, normal={2}, metallic={3}, roughness={4}",
                       materialResource->getName(),
                       materialResource->getAlbedoMapUUID() == -1 ? "none" : materialResource->getAlbedoMapUUID().toString(),
                       materialResource->getNormalMapUUID() == -1 ? "none" : materialResource->getNormalMapUUID().toString(),
                       materialResource->getMetallicMapUUID() == -1 ? "none" : materialResource->getMetallicMapUUID().toString(),
                       materialResource->getRoughnessMapUUID() == -1 ? "none" : materialResource->getRoughnessMapUUID().toString());
        return {materialResource, textures};
    }
}
