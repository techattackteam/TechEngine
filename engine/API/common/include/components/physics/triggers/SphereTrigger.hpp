#pragma once


#include <common/include/components/Component.hpp>
#include <glm/vec3.hpp>


namespace TechEngine {
    class SphereTrigger;
}

namespace TechEngineAPI {
    class SphereTrigger final : public Component {
    private:
        TechEngine::SphereTrigger* m_sphereTrigger = nullptr;

    public:
        explicit SphereTrigger(const Entity entity);

        void setCenter(glm::vec3 center) const;

        void setRadius(float radius) const;

        glm::vec3 getCenter() const;

        float getRadius() const;

        void updateInternalPointer(TechEngine::Scene* scene) override;
    };
}
