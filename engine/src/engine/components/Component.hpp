#pragma once

#include <string>
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"

using ComponentName = std::string;
namespace Engine {

    class Component {
    protected:
        ComponentName name;
    public:
        Component(ComponentName name);

        virtual ~Component();

        virtual void fixedUpdate();

        virtual void update();

        virtual void getInfo() = 0;

        ComponentName &getName() {
            return name;
        };
    };
}


