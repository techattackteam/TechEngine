#pragma once

#include <imGuizmo.h>
#include "scene/GameObject.hpp"

namespace TechEngine {
    class Guizmo {

    private:
        int operation = ImGuizmo::OPERATION::TRANSLATE;
        int mode = ImGuizmo::MODE::LOCAL;

    public:
        Guizmo();

        void editTransform(ImGuiContext *context, std::vector<GameObject *> &selectedGO);

        void setOperation(int operation);

        int getMode() const;

        void setMode(int mode);
    };
}
