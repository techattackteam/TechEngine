#pragma once

#include <TechEngine/platform/input/Key.hpp>
#include <TechEngine/platform/input/MouseButton.hpp>

namespace TechEngine {
    Key translateGlfwKey(int key);

    MouseButton translateGlfwMouseButton(int button);
}
