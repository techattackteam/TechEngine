#pragma once

#include <string>
#include <assimp/Importer.hpp>
#include "renderer/Texture.hpp"
#include "mesh/Mesh.hpp"
#include "material/MaterialManager.hpp"
#include "assimp/scene.h"

#include "scene/Scene.hpp"

namespace TechEngine {
    class AssimpLoader {
    private:
        Scene &scene;
        MaterialManager &materialManager;
    public:
        AssimpLoader(Scene &scene, MaterialManager &materialManager);

        GameObject *loadModel(const std::string &path);

    private:
        std::string directory;

        std::vector<GameObject *> processNode(const std::string &modelName, aiNode *node, const aiScene *aiScene);

        std::pair<Mesh *, Material *> processMesh(const std::string &modelName, aiMesh *mesh, const aiScene *scene);

    };
}
