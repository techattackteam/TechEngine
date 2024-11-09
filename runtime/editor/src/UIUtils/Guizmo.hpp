#pragma once

#include <imGuizmo.h>
#include <imgui_internal.h>
#include <vector>

#include "components/Components.hpp"

namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class Camera;

    class Guizmo {
    private:
        int operation = ImGuizmo::OPERATION::TRANSLATE;
        int mode = ImGuizmo::MODE::LOCAL;
        int& id;
        inline static int lastUsingID = -1;
        SystemsRegistry& m_systemsRegistry;

    public:
        Guizmo(int& id, SystemsRegistry& appSystemsRegistry);

        void editTransform(Camera* camera, ImGuiContext* context, const std::vector<Entity>& selectedEntities);

        void setOperation(int operation);

        ImGuizmo::MODE getMode() const;

        void setMode(int mode);
    };
}
