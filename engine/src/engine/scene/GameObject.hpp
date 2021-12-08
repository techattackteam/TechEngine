
#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>
#include <glm/glm.hpp>

namespace Engine {
    using ComponentName = std::string;

    class Component;

    class TransformComponent;

    class GameObject {
    private:
        std::unordered_map<ComponentName, Component *> components;

        std::string name;

    public:
        bool showInfoPanel = false;

        GameObject(std::string name, bool showInfoPanel = true);

        template<typename C, typename... A>
        void addComponent(A ...args) {
            if (!hasComponent<C>() && C::getName) {
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

        virtual void fixUpdate();

        virtual void update();

        glm::mat4 getModelMatrix();

        std::string getName();

        void showInfo();

        TransformComponent &getTransform();
    };
}


