#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class BloomPass : public IRenderPass {
    public:
        const char* name() const override {
            return "BloomPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
