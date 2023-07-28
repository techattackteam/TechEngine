#pragma once

#include <imGuizmo.h>
#include "scene/GameObject.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    class Guizmo {
    public:
        Guizmo();

        void editTransform(ImGuiContext *context);
    };
}
