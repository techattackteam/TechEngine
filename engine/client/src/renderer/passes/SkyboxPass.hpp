#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class SkyboxPass : public IRenderPass {
    public:
        const char* name() const override {
            return "SkyboxPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
