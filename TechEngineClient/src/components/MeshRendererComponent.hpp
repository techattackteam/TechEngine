#pragma once

#include <vector>
#include "components/Component.hpp"
#include "mesh/Mesh.hpp"
#include "material/Material.hpp"

namespace TechEngine {
    class MeshRendererComponent : public Component {
    private:
        Mesh *mesh;
        Material *m_material;

    public:
        MeshRendererComponent(GameObject *gameObject);

        MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material);

        void changeMesh(Mesh *mesh);

        void changeMaterial(Material &material);

        Mesh &getMesh();

        Material &getMaterial();

        std::vector<Vertex> getVertices();

        std::vector<int> getIndices();

        void paintMesh();

    };
}


