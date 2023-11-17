#pragma once

#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include "renderer/Texture.hpp"
#include "mesh/Mesh.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class AssimpLoader {
    public:
        GameObject *loadModel(const std::string &path);

    private:
        std::string directory;

        std::vector<GameObject *> processNode(const std::string &modelName, aiNode *node, const aiScene *scene);

        std::pair<Mesh *, Material *> processMesh(const std::string &modelName, aiMesh *mesh, const aiScene *scene);

    };
}
