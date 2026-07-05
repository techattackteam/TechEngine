#pragma once

#include <imGuizmo.h>
#include "../panels/hieraychyPanel/HierarchyNode.hpp"
#include "ui/Widget.hpp"

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

        void editTransform(Camera* camera, ImGuiContext* context, HierarchyNode& selectedNode);

        void editUI(ImVec2 viewPosition, ImVec2 viewSize, std::shared_ptr<Widget> widget);

        void setOperation(int operation);

        ImGuizmo::MODE getMode() const;

        void setMode(int mode);

        bool isGuizmoInUse() {
            return ImGuizmo::IsUsing();
        }
    };
}
