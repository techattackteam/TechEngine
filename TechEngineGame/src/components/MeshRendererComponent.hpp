#pragma once

#include <vector>
#include "components/Component.hpp"
#include "../renderer/VertexBuffer.hpp"
#include "../mesh/Mesh.hpp"
#include "../mesh/Material.hpp"

namespace TechEngine {
    class MeshRendererComponent : public Component {
    private:
        Mesh &mesh;
        Material &material;

    public:
        MeshRendererComponent(GameObject *gameObject, Mesh *mesh, Material *material);

        void changeMesh(Mesh &mesh);

        void getInfo() override;

        static ComponentName getName() {
            return "Mesh";
        }

        Material &getMaterial();

        std::vector<Vertex> getVertices();

    private:
        void paintMesh();

    };
}


