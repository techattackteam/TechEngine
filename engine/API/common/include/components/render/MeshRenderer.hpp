#pragma once

#include "common/include/components/Component.hpp"
#include "common/include/resources/Mesh.hpp"
#include "common/include/resources/Material.hpp"
#include <memory>

namespace TechEngine {
    class MeshRenderer;
}

namespace TechEngineAPI {
    class API_DLL MeshRenderer final : public Component {
    private:
        friend class Scene;
        TechEngine::MeshRenderer* m_meshRenderer;
        std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Material> m_material;
        bool update;

    public:
        MeshRenderer(const Entity entity,
                     const std::shared_ptr<Mesh>& mesh,
                     const std::shared_ptr<Material>& material): Component(entity),
                                                                 m_mesh(mesh),
                                                                 m_material(material) {
        }

        void updateInternalPointer(TechEngine::Scene* scene) override;

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

        void paintMesh();
    };
}
