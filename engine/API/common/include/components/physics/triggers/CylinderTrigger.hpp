#pragma once
#include <common/include/components/Component.hpp>
#include <glm/vec3.hpp>

namespace TechEngine {
    class CylinderTrigger;
}

namespace TechEngineAPI {
    class CylinderTrigger final : public Component {
    private:
        TechEngine::CylinderTrigger* m_cylinderTrigger = nullptr;

    public:
        explicit CylinderTrigger(const Entity entity);

        void setCenter(glm::vec3 center) const;

        void setHeight(float height) const;

        void setRadius(float radius) const;

        glm::vec3 getCenter() const;

        float getHeight() const;

        float getRadius() const;

        virtual void updateInternalPointer(TechEngine::Scene* scene);
    };
}
