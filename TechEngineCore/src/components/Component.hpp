#pragma once

#include <string>
#include <imgui.h>
#include "scene/GameObject.hpp"

namespace TechEngine {
    class GameObject;

    class Component {
    protected:
        std::string name;
        GameObject *gameObject;
    public:
        Component(GameObject *gameObject, std::string name);

        virtual ~Component();

        virtual void fixedUpdate();

        virtual void update();

        virtual std::string &getName() {
            return name;
        };

        TransformComponent &getTransform();

        GameObject *getGameObject();
    };
}


