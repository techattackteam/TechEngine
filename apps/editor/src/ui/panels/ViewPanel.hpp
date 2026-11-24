#pragma once

#include <TechEngine/client/render/RenderView.hpp>

#include <ui/panels/Panel.hpp>

#include <imgui.h>

#include <cstdint>
#include <string>

namespace TechEngine {
    class ViewPanel : public Panel {
    private:
        std::uint64_t m_token = 0;
        // In framebuffer pixels, measured by the last onDraw.
        int m_width = 0;
        int m_height = 0;

    public:
        ViewPanel(std::string name, std::uint64_t token);

        ViewRequest describe(bool visible) const;

    protected:
        void onDraw(const EditorSnapshot& snapshot) override;

        // Runs inside the panel's window after the image, and not at all while the panel is hidden. imageSize is in
        // ImGui's pixels, the same as its mouse positions, not the target's framebuffer pixels.
        virtual void updateCamera(const EditorSnapshot& snapshot, const ImVec2& imageSize, bool imageHovered);

        // Runs while the image is the last item, and not at all on a frame with no image to drop on.
        virtual void acceptDrop(const EditorSnapshot& snapshot);

        virtual void describeCamera(ViewRequest& request) const = 0;
    };
}
