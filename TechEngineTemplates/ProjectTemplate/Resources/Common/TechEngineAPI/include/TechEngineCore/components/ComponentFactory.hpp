#pragma once
#include "Component.hpp"


namespace TechEngine::ComponentFactory {
    Component* createComponent(std::string name, GameObject* gameObject, EventDispatcher& eventDispatcher);
}
