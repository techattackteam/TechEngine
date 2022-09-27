#pragma once


#include "components/Transform.hpp"

namespace TechEngine {
    class TransformComponent : public Transform {
    public:
        explicit TransformComponent(GameObject *gameObject);

        void drawDrag(const std::string &name, glm::vec3 &value);

        void getInfo();
    };
}
