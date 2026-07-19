#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class GeometryPass : public IRenderPass {
    public:
        const char* name() const override {
            return "GeometryPass";
        }

        bool isEnabled(const FrameContext& frame) const override;

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
