#pragma once

#include "Component.hpp"
#include <string>

#include "resources/Mesh.hpp"
#include "resources/Material.hpp"

namespace TechEngineAPI {
    class API_DLL MeshRenderer : public Component {
    private:
        friend class Scene;
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Material> m_material;
        bool update;

    public:
        MeshRenderer(const std::shared_ptr<Mesh>& mesh,
                     const std::shared_ptr<Material>& material): Component(),
                                                                 m_mesh(mesh),
                                                                 m_material(material) {
        }

        void setMesh(const std::shared_ptr<Mesh>& mesh) {
            this->m_mesh = mesh;
            update = true;
        }

        void setMaterial(const std::shared_ptr<Material>& material) {
            this->m_material = material;
            update = true;
        }

        [[nodiscard]] std::shared_ptr<Mesh> getMesh() const {
            return m_mesh;
        }

        [[nodiscard]] std::shared_ptr<Material> getMaterial() const {
            return m_material;
        }

        void paintMesh() {
            if (update) {
                update = false;
            }
        }
    };
}
