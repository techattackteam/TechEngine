#pragma once

#include <vector>
#include "Component.hpp"
#include "../renderer/VertexBuffer.hpp"

namespace Engine {
    class MeshComponent : public Component {
    private:
        std::vector<Vertex> vertices;
    public:
        MeshComponent();

        void createVertex(glm::vec3 localPosition, glm::vec3 offset, glm::vec3 normal, glm::vec4 color);

        std::vector<Vertex> getVertices();

        void getInfo() override;

        void createCube();

        static ComponentName getName() {
            return "Mesh";

        }

    };
}


