#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class ShadowDepthPass : public IRenderPass {
    public:
        const char* name() const override {
            return "ShadowDepthPass";
        }

        bool isEnabled(const FrameContext& frame) const override;

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
