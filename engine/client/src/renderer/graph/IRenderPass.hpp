#pragma once

namespace TechEngine {
    struct FrameContext;
    class RenderResources;

    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        virtual const char* name() const = 0;

        virtual void setup(RenderResources& resources) {
        }

        virtual bool isEnabled(const FrameContext& frame) const {
            return true;
        }

        virtual void execute(const FrameContext& frame, RenderResources& resources) = 0;
    };
}
