#pragma once

#include <vector>
#include "Component.hpp"
#include "../material/Material.hpp"
#include "../mesh/Mesh.hpp"

namespace TechEngine {
    class MeshRendererComponent : public Component {
    private:
        Mesh* mesh;
        Material* m_material = nullptr;

    public:
        MeshRendererComponent(GameObject* gameObject);

        MeshRendererComponent(GameObject* gameObject, Mesh* mesh, Material* material);

        void changeMesh(Mesh* mesh);

        void changeMaterial(Material& material);

        void changeMaterial(std::string& name);

        Mesh& getMesh();

        Material& getMaterial();

        std::vector<Vertex> getVertices();

        std::vector<int> getIndices();

        void paintMesh();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
