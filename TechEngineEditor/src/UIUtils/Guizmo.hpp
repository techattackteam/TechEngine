#pragma once

#include <imGuizmo.h>

#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    class Guizmo {
    private:
        int operation = ImGuizmo::OPERATION::TRANSLATE;
        int mode = ImGuizmo::MODE::LOCAL;
        PhysicsEngine& physicsEngine;

    public:
        Guizmo(PhysicsEngine& physicsEngine);

        void editTransform(CameraComponent* camera, ImGuiContext* context, std::vector<GameObject*>& selectedGO);

        void setOperation(int operation);

        int getMode() const;

        void setMode(int mode);
    };
}
