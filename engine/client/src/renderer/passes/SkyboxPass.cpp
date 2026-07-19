#include "SkyboxPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/texture/SkyBox.hpp"

namespace TechEngine {
    void SkyboxPass::execute(const FrameContext& frame, RenderResources& resources) {
        frame.skyBox->renderSkybox(resources.getFramebuffer(resources.gBufferFBO), frame.viewMatrix, frame.projectionMatrix);
    }
}
