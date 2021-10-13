#pragma once

#include <string>
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
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


