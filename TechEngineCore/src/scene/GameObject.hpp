
#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <typeinfo>

namespace TechEngine {
    class Component;

    class TransformComponent;

    class GameObject {
    private:
        GameObject *parent = nullptr;
        std::unordered_map<std::string, GameObject *> children;
        std::unordered_map<std::string, Component *> components;

        std::string name;
        std::string tag;
        bool stackAllocated = false;

    public:

        explicit GameObject(std::string name);

        GameObject() = default;

        virtual ~GameObject();

        template<class C, typename... A>
        void addComponent(A ...args) {
            if (!hasComponent<C>()) {
                C *component = new C(this, args...);
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

        void makeParent(GameObject *parent);

        void addChild(GameObject *child);

        void removeParent();

        void removeChild(const std::string &tag);

        glm::mat4 getModelMatrix();

        std::string getName();

        std::string getTag();

        GameObject* getParent();

        TransformComponent &getTransform();

        std::unordered_map<std::string, Component *> &getComponents();

        std::unordered_map<std::string, GameObject *> &getChildren();

        bool operator==(const GameObject *gameObject) {
            return name == gameObject->name;
        }

        void *operator new(size_t size) {
            void *p = ::operator new(size);
            ((GameObject *) p)->stackAllocated = true;
            return p;
        }

        void operator delete(void *p) {
            if (((GameObject *) p)->stackAllocated) {
                ::operator delete(p);
            }
        }

        void setTag(std::string basicString);

    private:
        void deleteChildren();
    };
}


