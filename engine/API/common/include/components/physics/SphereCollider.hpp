#pragma once


#include <common/include/components/Component.hpp>
#include <glm/vec3.hpp>


namespace TechEngine {
    class SphereCollider;
}

namespace TechEngineAPI {
    class SphereCollider final : public Component {
    private:
        TechEngine::SphereCollider* m_sphereCollider = nullptr;

    public:
        explicit SphereCollider(const Entity entity);

        void setCenter(glm::vec3 center) const;

        void setRadius(float radius) const;

        glm::vec3 getCenter() const;

        float getRadius() const;

        void updateInternalPointer(TechEngine::Scene* scene) override;
    };
}
