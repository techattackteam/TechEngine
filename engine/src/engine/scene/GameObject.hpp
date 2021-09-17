#pragma once

#include <string>
#include <unordered_map>
#include "../components/Component.hpp"
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include <glm/glm.hpp>

namespace Engine {
    class GameObject {
    private:
        std::unordered_map<ComponentName, Component *> components;
    public:
        bool showInfoPanel = false;

        explicit GameObject(bool showInfoPanel = true);

        template<typename C>
        void addComponent() {
            if (!components.contains(C::getName())) {
                C *component = new C();
                components[C::getName()] = component;
            }
        }

        template<typename C, typename... A>
        void addComponent(A ...args) {
            if (!components.contains(C::getName())) {
                C *component = new C(args...);
                components[C::getName()] = component;
            }
        }

        template<typename C>
        void removeComponent() {
        }

        template<typename C>
        bool hasComponent() {
            return components.contains(C::getName());
        }

        template<class C>
        C *getComponent() {
            return (C *) components[C::getName()];
        }

        //void enableComponent(Component &component);

        //void disableComponent(Component &component);

        virtual void update();

        glm::mat4 getModelMatrix();

        void showInfo();


    };
}


