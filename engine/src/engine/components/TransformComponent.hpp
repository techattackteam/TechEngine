#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/common.hpp>
#include "../../../lib/imgui/imgui_internal.h"
#include <glm/gtc/type_ptr.hpp>
#include "Component.hpp"

namespace Engine {
    class TransformComponent : public Component {
    public:
        glm::vec3 position;
        glm::vec3 orientation;
        glm::vec3 scale;
        glm::mat4 model;

        TransformComponent();

        glm::mat4 getModelMatrix();

        void getInfo() override;

        glm::vec3 getPosition() const;


        glm::vec3 getOrientation() const;

        static ComponentName getName() {
            return "Transform";
        }

        static void drawDrag(const std::string &name, glm::vec3 &value);

        void translate(glm::vec3 vector);

        void translateTo(glm::vec3 position);

        void rotate(glm::vec3 rotation);

        void setScale(glm::vec3 vector);

        void rotate(glm::quat quaternion);

        void lookAt(glm::vec3 position);
    };
}


