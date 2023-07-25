#pragma once

#include <imGuizmo.h>
#include "scene/GameObject.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    class Guizmo {
    private:

        GameObject *selectedObject = nullptr;

    public:
        Guizmo();

        void editTransform(ImGuiContext *context);

        void onGameObjectDestroyEvent(TechEngine::GameObjectDestroyEvent *event);
    };
}
