
#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>
#include <glm/glm.hpp>
#include <typeinfo>

namespace TechEngine {
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

        template<class C, typename... A>
        void addComponent(A ...args) {
            if (!hasComponent<C>()) {
                C *component = new C(args...);
                components[typeid(C).name()] = component;
            }
        }

        template<typename C>
        void removeComponent() {
            if (hasComponent<C>()) {
                components.erase(typeid(C).name());
            }
        }

        template<typename C>
        bool hasComponent() {
            return components.contains(typeid(C).name());
        }

        template<class C>
        C *getComponent() {
            return (C *) components[typeid(C).name()];
        }

        virtual void fixUpdate();

        virtual void update();

        glm::mat4 getModelMatrix();

        std::string getName();

        void showInfo();

        TransformComponent &getTransform();
    };
}


