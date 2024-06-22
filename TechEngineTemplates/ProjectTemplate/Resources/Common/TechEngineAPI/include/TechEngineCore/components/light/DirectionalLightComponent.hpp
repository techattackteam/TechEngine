#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "components/Component.hpp"


namespace TechEngine {
    class CORE_DLL DirectionalLightComponent : public Component {
    private:
        glm::vec4 color;

        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f);

    public:
        DirectionalLightComponent(GameObject* gameObject, SystemsRegistry& systemsRegistry);

        void fixedUpdate() override;

        glm::mat4& getProjectionMatrix();

        glm::mat4& getViewMatrix();

        glm::vec4& getColor();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
