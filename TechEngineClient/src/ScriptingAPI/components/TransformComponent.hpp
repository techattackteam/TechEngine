#pragma once

#include <glm/glm.hpp>

namespace TechEngine {
    class TransformComponent;
}

namespace TechEngineAPI {
    class TransformComponent {
    private:
        TechEngine::TransformComponent* transformComponent = nullptr;

    public:
        explicit TransformComponent(TechEngine::TransformComponent* transformComponent);

        void setPosition(glm::vec3 position);

        void setRotation(glm::vec3 rotation);

        void setScale(glm::vec3 scale);

        void translate(glm::vec3 translation);

        void rotate(glm::vec3 rotation);

        void lookAt(glm::vec3 position);

        glm::vec3 getPosition() const;

        glm::vec3 getRotation() const;

        glm::vec3 getScale() const;
    };
}
