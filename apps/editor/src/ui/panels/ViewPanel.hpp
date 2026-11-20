#pragma once

#include <TechEngine/client/render/RenderView.hpp>

#include <ui/panels/Panel.hpp>

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

        // Runs inside the panel's window after the image, and not at all while the panel is hidden.
        virtual void updateCamera(const EditorSnapshot& snapshot, bool imageHovered);

        virtual void describeCamera(ViewRequest& request) const = 0;
    };
}
