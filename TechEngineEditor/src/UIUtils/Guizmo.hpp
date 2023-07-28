#pragma once

#include <imGuizmo.h>
#include "scene/GameObject.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    class Guizmo {

    private:
        int operation = ImGuizmo::OPERATION::TRANSLATE;
    public:
        Guizmo();

        void editTransform(ImGuiContext *context);

        void setOperation(int operation);
    };
}
