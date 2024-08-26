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
    AssimpLoader::Node AssimpLoader::loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* aiScene = importer.ReadFile(path, aiProcess_Triangulate);

        if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode) {
            TE_LOGGER_WARN("Assimp error: {}", importer.GetErrorString());
        }

        // Process the scene and create a game object
        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials and {2} textures", aiScene->mNumMeshes, aiScene->mNumMaterials, aiScene->mNumTextures);
        Node node = processScene(FileUtils::getFileName(path), aiScene->mRootNode, aiScene, false);
        return node;
    }

    AssimpLoader::Node AssimpLoader::processScene(const std::string& modelName, aiNode* aiNode, const aiScene* aiScene, bool _return) {
        Node node;
        for (unsigned int i = 0; i < aiNode->mNumMeshes; i++) {
            aiVector3t<float> scaling;
            aiVector3t<float> rotation;
            aiVector3t<float> position;
            aiNode->mTransformation.Decompose(scaling, rotation, position);
            aiMesh* aiMesh = aiScene->mMeshes[aiNode->mMeshes[i]];

            MeshData processedMesh = processMesh(modelName, aiMesh, aiScene);
            node.meshes.emplace_back(processedMesh);
        }
        for (unsigned int i = 0; i < aiNode->mNumChildren; i++) {
            Node child = processScene(modelName, aiNode->mChildren[i], aiScene, true);
            node.children.emplace_back(child);
        }
        return node;
    }

    AssimpLoader::MeshData AssimpLoader::processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene) {
        MeshData meshData;
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
        if (mesh->mMaterialIndex >= 0) {
            meshData.material = modelName + "-" + scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
            TE_LOGGER_INFO("Material: {}", meshData.material);
        } else {
            meshData.material = modelName + "-default";
        }
        return meshData;
    }
}
