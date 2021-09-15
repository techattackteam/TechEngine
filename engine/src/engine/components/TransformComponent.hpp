#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Component.hpp"

namespace Engine {
    class TransformComponent : public Component {
    public:
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;

        TransformComponent();

        glm::mat4 getModelMatrix();

        void getInfo() override;
    };
}


