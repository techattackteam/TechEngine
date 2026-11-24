#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <edit/camera/FlyCamera.hpp>
#include <ui/panels/ViewPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    class SceneViewPanel final : public ViewPanel {
    private:
        FlyCamera m_camera;
        bool m_looking = false;
        bool m_panning = false;

    public:
        SceneViewPanel();

        // While true the window's cursor must be hidden and locked, so a rotation or a pan never stops at the screen's
        // edge. ImGui's TeleportMousePos cannot do it: ImGui drops every mouse position event in the frame after a
        // teleport.
        bool wantsCursorLocked() const;

        // While true main must build a frame even with no input, or an easing zoom, focus or rotation stalls.
        bool isCameraMoving() const;

    protected:
        void updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, bool imageHovered) override;

        void describeCamera(ViewRequest& request) const override;

    private:
        void trackDrags(bool imageHovered);

        static bool findSelected(const EditorSnapshot& snapshot, Vec3& position, float& radius);
    };
}
