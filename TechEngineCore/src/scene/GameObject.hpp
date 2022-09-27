
#pragma once

#include <string>
#include <unordered_map>
#include <imgui.h>
#include <glm/glm.hpp>

namespace TechEngine {
    using ComponentName = std::string;

    class Component;

    class Transform;

    class GameObject {
    private:
        std::unordered_map<ComponentName, Component *> components;

        std::string name;

    public:
        bool showInfoPanel = false;

        GameObject(std::string name, bool showInfoPanel = true);

        template<class C, typename... A>
        void addComponent(A ...args) {
            if (!hasComponent<C>() && C::getName) {
                C *component = new C(args...);
                components[C::getName()] = component;
            }
        }

        template<typename C>
        void removeComponent() {
            if (hasComponent<C>() && C::getName) {
                components.erase(C::getName());
            }
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

        Transform &getTransform();
    };
}


