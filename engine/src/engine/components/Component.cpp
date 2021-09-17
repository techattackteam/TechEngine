#include "Component.hpp"

#include <utility>
#include <glm/detail/type_vec3.hpp>

namespace Engine {
    Component::Component(ComponentName name) : name(std::move(name)) {

    }

    Component::~Component() {

    }

}
