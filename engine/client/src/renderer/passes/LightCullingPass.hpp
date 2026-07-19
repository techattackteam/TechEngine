#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class LightCullingPass : public IRenderPass {
    public:
        const char* name() const override {
            return "LightCullingPass";
        }

        bool isEnabled(const FrameContext& frame) const override;

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
