#pragma once

#include "Component.hpp"
#include <glm/vec3.hpp>


namespace TechEngineAPI {
    class Transform : public Component {
    private:
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;

    public:
        Transform(const glm::vec3& position,
                  const glm::vec3& rotation,
                  const glm::vec3& scale): Component(),
                                           position(position),
                                           rotation(rotation),
                                           scale(scale) {
        }


        void setPosition(const glm::vec3& position) {
            this->position = position;
        }

        void setRotation(const glm::vec3& rotation) {
            this->rotation = rotation;
        }

        void setScale(const glm::vec3& scale) {
            this->scale = scale;
        }

        [[nodiscard]] glm::vec3 getPosition() const {
            return position;
        }

        [[nodiscard]] glm::vec3 getRotation() const {
            return rotation;
        }

        [[nodiscard]] glm::vec3 getScale() const {
            return scale;
        }
    };
}
