#pragma once

#include <vector>
#include "components/Component.hpp"
#include "mesh/Mesh.hpp"
#include "mesh/Material.hpp"

namespace TechEngine {
    class MeshRendererComponent : public Component {
    private:
        Mesh *mesh;
        Material &material;

    public:
        MeshRendererComponent(GameObject *gameObject);

        MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material);

        void changeMesh(Mesh *mesh);

        Mesh &getMesh();

        Material &getMaterial();

        std::vector<Vertex> getVertices();

        std::vector<int> getIndices();

        void paintMesh();

    };
}


