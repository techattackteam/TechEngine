#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <typeinfo>
#include <string>

namespace TechEngine {
    class Component;

    class TransformComponent;

    class GameObject {
    private:
        GameObject* parent = nullptr;
        std::unordered_map<std::string, GameObject*> children;
        std::unordered_map<std::string, Component*> components;

        std::string name;
        std::string tag;
        bool stackAllocated = false;
        bool editorOnly = false;

    protected:

        explicit GameObject(std::string name);

        GameObject(std::string name, const std::string &tag);

        GameObject(GameObject* gameObject, const std::string& tag);

        friend class Scene;

    public:
        virtual ~GameObject();

        template<class C, typename... A>
        void addComponent(A&&... args) {
            if (!hasComponent<C>()) {
                C* component = new C(this, args...);
                components[typeid(C).name()] = component;
            }
        }

        template<typename C>
        void removeComponent() {
            if (hasComponent<C>()) {
                C* component = getComponent<C>();
                components.erase(typeid(C).name());
                delete component;
            }
        }

        template<typename C>
        bool hasComponent() {
            return components.contains(typeid(C).name());
        }

        bool hasComponent(const std::string& name) {
            return components.contains(name);
        }

        template<class C>
        C* getComponent() {
            return (C*)components[typeid(C).name()];
        }

        virtual void fixUpdate();

        virtual void update();

        void makeParent(GameObject* parent);

        void addChild(GameObject* child);

        void removeParent();

        void removeChild(const std::string& tag);

        glm::mat4 getModelMatrix();

        glm::mat4 getLocalModelMatrix();

        std::string getName();

        std::string getTag();

        GameObject* getParent();

        TransformComponent& getTransform();

        std::unordered_map<std::string, Component*>& getComponents();

        std::unordered_map<std::string, GameObject*>& getChildren();

        bool operator==(const GameObject* gameObject) {
            return tag == gameObject->tag;
        }

        void* operator new(size_t size) {
            void* p = ::operator new(size);
            ((GameObject*)p)->stackAllocated = true;
            return p;
        }

        void operator delete(void* p) {
            if (((GameObject*)p)->stackAllocated) {
                ::operator delete(p);
            }
        }

        void setTag(std::string basicString);

        bool hasParent();

        bool hasChildren();

        void setEditorOnly(bool editorOnly);

        bool isEditorOnly() const;

    private:
        void deleteChildren();
    };
}
