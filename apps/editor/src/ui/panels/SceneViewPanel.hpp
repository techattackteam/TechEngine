#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <edit/FlyCamera.hpp>
#include <ui/panels/ViewPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    class EditorCommands;

    class SceneViewPanel final : public ViewPanel {
    private:
        EditorCommands* m_commands = nullptr;
        FlyCamera m_camera;
        bool m_looking = false;
        bool m_panning = false;

    public:
        explicit SceneViewPanel(EditorCommands& commands);

        // While true the window's cursor must be hidden and locked, so a rotation or a pan never stops at the screen's
        // edge. ImGui's TeleportMousePos cannot do it: ImGui drops every mouse position event in the frame after a
        // teleport.
        bool wantsCursorLocked() const;

        // While true main must build a frame even with no input, or an easing zoom, focus or rotation stalls.
        bool isCameraMoving() const;

    protected:
        void updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, bool imageHovered) override;

        void describeCamera(ViewRequest& request) const override;

        void acceptDrop(const EditorSnapshot& snapshot) override;

    private:
        void trackDrags(bool imageHovered);

        static bool findSelected(const EditorSnapshot& snapshot, Vec3& position, float& radius);
    };
}
