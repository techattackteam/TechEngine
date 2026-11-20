#pragma once

#include <TechEngine/base/math/Math.hpp>

#include <edit/FlyCamera.hpp>
#include <ui/panels/ViewPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    class SceneViewPanel final : public ViewPanel {
    private:
        FlyCamera m_camera;
        bool m_looking = false;
        bool m_panning = false;
        ImVec2 m_lookAnchor = ImVec2(0.0f, 0.0f);

    public:
        SceneViewPanel();

    protected:
        void updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, bool imageHovered) override;

        void describeCamera(ViewRequest& request) const override;

    private:
        void trackDrags(bool imageHovered);

        void readLookInput(FlyCameraInput& input);

        static bool findSelected(const EditorSnapshot& snapshot, Vec3& position, float& radius);

        static float getKeyAxis(ImGuiKey positive, ImGuiKey negative);
    };
}
