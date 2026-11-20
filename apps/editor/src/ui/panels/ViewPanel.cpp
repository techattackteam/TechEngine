#include <ui/panels/ViewPanel.hpp>

#include <imgui.h>

#include <utility>

namespace TechEngine {
    ViewPanel::ViewPanel(std::string name, const std::uint64_t token) : Panel(std::move(name)), m_token(token) {
        addStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    ViewRequest ViewPanel::describe(const bool visible) const {
        ViewRequest request;
        request.token = m_token;
        request.width = m_width;
        request.height = m_height;
        request.visible = visible;
        describeCamera(request);
        return request;
    }

    void ViewPanel::onDraw(const EditorSnapshot&) {
        const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
        const ImGuiIO& io = ImGui::GetIO();
        m_width = static_cast<int>(contentRegion.x * io.DisplayFramebufferScale.x);
        m_height = static_cast<int>(contentRegion.y * io.DisplayFramebufferScale.y);
        if (contentRegion.x <= 0.0f || contentRegion.y <= 0.0f) {
            return;
        }
        // A GL target's first row is its bottom, so V runs from 1 at the top to 0.
        ImGui::Image(ImTextureRef(m_token), contentRegion, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
}
