#include "UIPass.hpp"

#include <memory>
#include <vector>

#include "renderer/graph/FrameContext.hpp"
#include "renderer/ui/UIRenderer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/PanelWidget.hpp"
#include "ui/WidgetsRegistry.hpp"

namespace TechEngine {
    void UIPass::execute(const FrameContext& frame, RenderResources& resources) {
        m_uiRenderer.beginFrame();
        PanelWidget rootWidget;
        rootWidget.m_name = "Root";
        rootWidget.m_finalScreenRect = {0.0f, 0.0f, (float)m_uiRenderer.m_screenWidth, (float)m_uiRenderer.m_screenHeight};

        std::vector<std::shared_ptr<Widget>>& widgets = frame.systems->getSystem<WidgetsRegistry>().getWidgets();
        for (auto it = widgets.rbegin(); it != widgets.rend(); it++) {
            auto& widget = *it;
            if (widget) {
                widget->draw(m_uiRenderer);
            }
        }
        m_uiRenderer.endFrame();
    }
}
