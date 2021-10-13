#pragma once

#include <vector>
#include "Component.hpp"
#include "../renderer/VertexBuffer.hpp"
#include "TransformComponent.hpp"
#include "../mesh/Mesh.hpp"

namespace Engine {
    class MeshRendererComponent : public Component {
    private:
        TransformComponent *transform;
        Mesh &mesh;
    public:
        MeshRendererComponent(GameObject *gameObject, Mesh *mesh);

        void changeMesh(Mesh &mesh);

        void getInfo() override;

        static ComponentName getName() {
            return "Mesh";

        }

        Mesh &getMesh();

        void renderMesh();
    };
}


