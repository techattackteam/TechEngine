#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class GBufferPass : public IRenderPass {
    public:
        const char* name() const override {
            return "GBufferPass";
        }

        bool isEnabled(const FrameContext& frame) const override;

        void execute(const FrameContext& frame, RenderResources& resources) override;

    private:
        void prepare(const FrameContext& frame, RenderResources& resources);
    };
}
