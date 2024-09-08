#pragma once


#include "systems/System.hpp"
#include "components/Archetype.hpp"

#include <glm/gtc/quaternion.hpp>

namespace TechEngine {
    class TransformSystem : public System {
    private:
        SystemsRegistry& m_systemsRegistry;

    public:
        explicit TransformSystem(SystemsRegistry& systemsRegistry);

        void translate(Entity entity, glm::vec3 vector);

        void translateTo(Entity entity, glm::vec3 position);

        void translateToWorld(Entity entity, glm::vec3 worldPosition);

        void setRotation(Entity entity, glm::vec3 rotation);

        void rotate(Entity entity, glm::vec3 rotation);

        void setRotation(Entity entity, glm::quat quaternion);

        void setScale(Entity entity, glm::vec3 vector);

        glm::mat4 getModelMatrix(int entity);
    };
}
