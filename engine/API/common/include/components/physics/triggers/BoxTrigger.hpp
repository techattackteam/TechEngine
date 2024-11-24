#pragma once

#include <common/include/components/Component.hpp>
#include <glm/vec3.hpp>

namespace TechEngine {
    class BoxTrigger;
}

namespace TechEngineAPI {
    class BoxTrigger : public Component {
    private:
        TechEngine::BoxTrigger* m_boxTrigger = nullptr;

    public:
        explicit BoxTrigger(const Entity entity): Component(entity) {
        }

        void setCenter(glm::vec3 center) const;

        void setSize(glm::vec3 size) const;

        glm::vec3 getCenter() const;

        glm::vec3 getSize() const;

        virtual void updateInternalPointer(TechEngine::Scene* scene);
    };
}
