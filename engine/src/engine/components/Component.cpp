#include "Component.hpp"

#include <utility>

namespace Engine {
    Component::Component(ComponentName name) : name(std::move(name)) {

    }

    ComponentName &Component::getName() {
        return name;
    }
}
