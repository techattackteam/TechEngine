#pragma once

#include <imGuizmo.h>

namespace TechEngine {
    class Guizmo {

    private:
        int operation = ImGuizmo::OPERATION::TRANSLATE;
        int mode = ImGuizmo::MODE::LOCAL;

    public:
        Guizmo();

        void editTransform(ImGuiContext *context);

        void setOperation(int operation);

        int getMode() const;

        void setMode(int mode);
    };
}
