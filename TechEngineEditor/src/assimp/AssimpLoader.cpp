#include "AssimpLoader.hpp"
#include "core/Logger.hpp"
#include "components/MeshRendererComponent.hpp"
#include "core/FileSystem.hpp"
#include "scene/Scene.hpp"
#include "mesh/ImportedMesh.hpp"
#include "renderer/TextureManager.hpp"
#include "material/MaterialManager.hpp"
#include <assimp/postprocess.h>

namespace TechEngine {
    GameObject *AssimpLoader::loadModel(const std::string &path) {
        Assimp::Importer importer;
        directory = std::filesystem::path(path).parent_path().string();
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            TE_LOGGER_WARN("Assimp error: {}", importer.GetErrorString());
            return nullptr;
        }

        // Process the scene and create a game object
        GameObject *gameObject = new GameObject(FileSystem::getFileName(path));
        std::vector<GameObject *> children = processNode(FileSystem::getFileName(path), scene->mRootNode, scene);
        TE_LOGGER_INFO("Loaded Model with {0} meshes, {1} materials and {2} textures", scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures);
        for (GameObject *child: children) {
            Scene::getInstance().makeChildTo(gameObject, child);
        }

        return gameObject;
    }

    std::vector<GameObject *> AssimpLoader::processNode(const std::string &modelName, aiNode *node, const aiScene *scene) {
        std::vector<GameObject *> gameObjects;
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiVector3t<float> scaling;
            aiVector3t<float> rotation;
            aiVector3t<float> position;
            node->mTransformation.Decompose(scaling, rotation, position);
            aiMesh *aiMesh = scene->mMeshes[node->mMeshes[i]];

            std::pair<Mesh *, Material *> processedMesh = processMesh(modelName, aiMesh, scene);
            GameObject *gameObject = new GameObject(aiMesh->mName.C_Str());
            gameObject->addComponent<MeshRendererComponent>(processedMesh.first, processedMesh.second);
            aiNode *parentNode = node->mParent;
            while (parentNode->mParent != nullptr) {
                aiVector3t<float> parentRotation;
                aiVector3t<float> parentPosition;
                parentNode->mTransformation.Decompose(scaling, rotation, position);
                position += parentPosition;
                rotation += parentRotation;
            }
            gameObject->getTransform().translateTo(glm::vec3(position.x / scaling.x, position.y / scaling.y, position.z / scaling.z));
            gameObject->getTransform().setRotation(glm::vec3(rotation.x, rotation.y, rotation.z));

            gameObjects.push_back(gameObject);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            std::vector<GameObject *> game_objects = processNode(modelName, node->mChildren[i], scene);
            gameObjects.insert(gameObjects.end(), game_objects.begin(), game_objects.end());
        }
        return gameObjects;
    }

    std::pair<Mesh *, Material *> AssimpLoader::processMesh(const std::string &modelName, aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<int> indices;
        std::vector<Texture *> textures;
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
        Material *material;
        if (mesh->mMaterialIndex >= 0) {
            for (int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
            std::string materialName = modelName + "-" + scene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();
            if (MaterialManager::materialExists(materialName)) {
                material = &MaterialManager::getMaterial(materialName);
            } else {
                material = &MaterialManager::createMaterialFile(materialName, directory);
            }
        }

        return std::make_pair(new ImportedMesh(vertices, indices), material);
    }


}
