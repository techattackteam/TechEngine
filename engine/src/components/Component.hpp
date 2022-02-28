#pragma once

#include <string>
#include <imgui.h>
#include "../scene/GameObject.hpp"

using ComponentName = std::string;

namespace Engine {
    class GameObject;

    class Component {
    protected:
        ComponentName name;
        GameObject *gameObject;
    public:
        Component(GameObject *gameObject, ComponentName name);

        virtual ~Component();

        virtual void fixedUpdate();

        virtual void update();

        virtual void getInfo() = 0;

        ComponentName &getName() {
            return name;
        };

        TransformComponent &getTransform();
    };
}


