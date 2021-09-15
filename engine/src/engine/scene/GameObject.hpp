#pragma once

#include <string>
#include <unordered_map>
#include "../components/Component.hpp"
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"

namespace Engine {
    class GameObject {
    private:
        std::unordered_map<ComponentName, Component *> components;
    public:
        bool showInfoPanel = false;

        GameObject(bool showInfoPanel = true);

        template<typename C>
        void addComponent();

        template<typename C, typename A>
        void addComponent(A args...);

        void removeComponent(ComponentName &name);

        template<typename C>
        bool hasComponent();

        template<typename C>
        bool getComponent();

        //void enableComponent(Component &component);

        //void disableComponent(Component &component);

        virtual void update();

        glm::mat4 getModelMatrix();

        void showInfo();


    };
}


