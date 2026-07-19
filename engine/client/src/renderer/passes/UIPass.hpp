#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class UIRenderer;

    class UIPass : public IRenderPass {
    private:
        UIRenderer& m_uiRenderer;

    public:
        explicit UIPass(UIRenderer& uiRenderer) : m_uiRenderer(uiRenderer) {
        }

        const char* name() const override {
            return "UIPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
