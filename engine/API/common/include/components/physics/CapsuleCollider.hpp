#pragma once

#include <common/include/components/Component.hpp>
#include <glm/vec3.hpp>

namespace TechEngine {
    class CapsuleCollider;
}

namespace TechEngineAPI {
    class CapsuleCollider final : public Component {
    private:
        TechEngine::CapsuleCollider* m_capsuleCollider = nullptr;

    public:
        explicit CapsuleCollider(const Entity entity);

        void setCenter(glm::vec3 center) const;

        void setHeight(float size) const;

        void setRadius(float radius) const;

        glm::vec3 getCenter() const;

        float getHeight() const;

        float getRadius() const;

        void updateInternalPointer(TechEngine::Scene* scene) override;
    };
}
