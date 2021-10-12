
#pragma once

#include <string>
#include <unordered_map>
#include "../components/Component.hpp"
#include "../../../lib/imgui/imgui.h"
#include "../../../lib/imgui/imgui_impl_opengl3.h"
#include "../../../lib/imgui/imgui_impl_glfw.h"
#include "../components/TransformComponent.hpp"
#include <glm/glm.hpp>

namespace Engine {
    using ComponentName = std::string;

    class GameObject {
    private:
        std::unordered_map<ComponentName, Component *> components;

        std::string name;
    public:
        bool showInfoPanel = false;

        explicit GameObject(std::string name, bool showInfoPanel = true);

        template<typename C>
        void addComponent() {
            if (!hasComponent<C>()) {
                C *component = new C();
                components[C::getName()] = component;
            }
        }

        template<typename C, typename... A>
        void addComponent(A ...args) {
            if (!hasComponent<C>()) {
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

        virtual void fixUpdate();

        virtual void update();

        glm::mat4 getModelMatrix();

        std::string getName();

        void showInfo();

        Engine::TransformComponent &getTransform();
    };
}


