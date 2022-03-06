#pragma once

#include <vector>
#include "Component.hpp"
#include "../renderer/VertexBuffer.hpp"
#include "../mesh/Mesh.hpp"
#include "../mesh/Material.hpp"

namespace Engine {
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

        Mesh &getMesh();

        Material &getMaterial();
    };
}


