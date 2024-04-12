#include "AssimpLoader.hpp"
#include "core/Logger.hpp"
#include "components/render/MeshRendererComponent.hpp"
#include "core/FileSystem.hpp"
#include "mesh/ImportedMesh.hpp"
#include "material/MaterialManager.hpp"
#include <assimp/postprocess.h>

namespace TechEngine {
    AssimpLoader::AssimpLoader(Scene& scene, MaterialManager& materialManager) : scene(scene), materialManager(materialManager) {
    }


    GameObject* AssimpLoader::loadModel(const std::string& path) {
        Assimp::Importer importer;
        directory = std::filesystem::path(path).parent_path().string();
        const aiScene* aiScene = importer.ReadFile(path, aiProcess_Triangulate);

        if (!aiScene || aiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiScene->mRootNode) {
            TE_LOGGER_WARN("Assimp error: {}", importer.GetErrorString());
            return nullptr;
        }

        // Process the scene and create a game object
        GameObject& gameObject = scene.createGameObject<GameObject>(FileSystem::getFileName(path));
        std::vector<GameObject*> children = processNode(FileSystem::getFileName(path), aiScene->mRootNode, aiScene);
        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials and {2} textures", aiScene->mNumMeshes, aiScene->mNumMaterials, aiScene->mNumTextures);
        for (GameObject* child: children) {
            scene.makeChildTo(&gameObject, child);
        }

        return &gameObject;
    }

    std::vector<GameObject*> AssimpLoader::processNode(const std::string& modelName, aiNode* node, const aiScene* aiScene) {
        std::vector<GameObject*> gameObjects;
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiVector3t<float> scaling;
            aiVector3t<float> rotation;
            aiVector3t<float> position;
            node->mTransformation.Decompose(scaling, rotation, position);
            aiMesh* aiMesh = aiScene->mMeshes[node->mMeshes[i]];

            std::pair<Mesh*, Material*> processedMesh = processMesh(modelName, aiMesh, aiScene);
            GameObject& gameObject = scene.createGameObject<GameObject>(aiMesh->mName.C_Str());
            gameObject.addComponent<MeshRendererComponent>(processedMesh.first, processedMesh.second);
            gameObject.getTransform().translateTo(glm::vec3(position.x / scaling.x, position.y / scaling.y, position.z / scaling.z));
            gameObject.getTransform().setRotation(glm::degrees(glm::vec3(rotation.x, rotation.y, rotation.z)));

            gameObjects.push_back(&gameObject);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            std::vector<GameObject*> game_objects = processNode(modelName, node->mChildren[i], aiScene);
            gameObjects.insert(gameObjects.end(), game_objects.begin(), game_objects.end());
        }
        return gameObjects;
    }

    std::pair<Mesh*, Material*> AssimpLoader::processMesh(const std::string& modelName, aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        std::vector<Texture*> textures;
        std::vector<int> materialsIndices;
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
            vertices.emplace_back(position, normal, textureCoordinate);
        }
        Material* material;
        if (mesh->mMaterialIndex >= 0) {
            for (int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
            std::string materialName = modelName + "-" + scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
            if (materialManager.materialExists(materialName)) {
                material = &materialManager.getMaterial(materialName);
            } else {
                material = &materialManager.createMaterialFile(materialName, directory);
            }
        }

        return std::make_pair(new ImportedMesh(vertices, indices), material);
    }
}
