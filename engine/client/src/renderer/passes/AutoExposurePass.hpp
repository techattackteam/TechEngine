#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class AutoExposurePass : public IRenderPass {
    private:
        float m_currentExposure = 1.0f;
        float m_targetExposure = 1.0f;
        float m_adaptationSpeed = 1.5f;

    public:
        const char* name() const override {
            return "AutoExposurePass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
