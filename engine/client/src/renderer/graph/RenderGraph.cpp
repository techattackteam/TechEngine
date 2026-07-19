#include "RenderGraph.hpp"

#include "FrameContext.hpp"
#include "RenderResources.hpp"

namespace TechEngine {
    void RenderGraph::setup(RenderResources& resources) {
        for (auto& pass: m_passes) {
            pass->setup(resources);
        }
    }

    void RenderGraph::execute(const FrameContext& frame, RenderResources& resources) {
        for (auto& pass: m_passes) {
            if (!pass->isEnabled(frame)) {
                continue;
            }
            pass->execute(frame, resources);
        }
    }

    void RenderGraph::clear() {
        m_passes.clear();
    }
}
