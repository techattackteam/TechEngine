#pragma once

#include <string>
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"

namespace Engine {
    using ComponentName = std::string;

    class Component {
    protected:
        ComponentName name;
    public:
        explicit Component(ComponentName name);

        virtual ~Component();

        virtual void update();

        virtual void getInfo() = 0;

        ComponentName &getName() {
            return name;
        };


    };
}


