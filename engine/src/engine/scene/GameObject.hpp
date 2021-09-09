#pragma once

#include <string>
#include <unordered_map>
#include "Component.hpp"

namespace Engine {
    class GameObject {
    private:
        std::unordered_map<ComponentName, Component &> components;
    protected:
        bool showInfoPanel = false;
    public:
        template<class C>
        void addComponent(C &component);

        void removeComponent(ComponentName &name);

        //void enableComponent(Component &component);

        //void disableComponent(Component &component);
    };
}


