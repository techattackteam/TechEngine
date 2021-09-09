#include "GameObject.hpp"

namespace Engine {

    template<class C>
    void GameObject::addComponent(C &component) {
        if (!components.contains(component.getName())) {
            components[component.getName()] = component;
        }
    }

    void GameObject::removeComponent(std::string &name) {
        components.erase(std::remove_if(components.begin(), components.end(), name), components.end());
    }
}
