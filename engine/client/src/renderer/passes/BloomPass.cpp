#include "BloomPass.hpp"

#include <cmath>

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderSettings.hpp"
#include "renderer/resources/GpuResourceManager.hpp"

namespace TechEngine {
    void BloomPass::execute(const FrameContext& frame, RenderResources& resources) {
        const glm::ivec2 viewport = frame.viewportSize;
        const BloomProperties& bloomProperties = frame.settings->bloom;

        resources.recreateBloomTexture(viewport);
        frame.gpu->changeActiveShader("bloomPrefilter");
        frame.gpu->getActiveShader()->setUniformFloat("u_threshold", bloomProperties.threshold);
        frame.gpu->getActiveShader()->setUniformFloat("u_knee", bloomProperties.knee);

        glActiveTexture(GL_TEXTURE0);
        FrameBuffer& hdrFBO = resources.getFramebuffer(resources.gBufferFBO);
        glBindTexture(GL_TEXTURE_2D, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT3));
        frame.gpu->getActiveShader()->setUniformInt("u_hdrBuffer", 0);


        glBindImageTexture(0, resources.bloomTexture.getID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glDispatchCompute((viewport.x + 15) / 16, (viewport.y + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glm::ivec2 mipSize = viewport;

        frame.gpu->changeActiveShader("bloomDownSample");

        for (int i = 0; i < resources.bloomIterations - 1; i++) {
            mipSize /= 2;
            if (mipSize.x == 0 || mipSize.y == 0) break;

            frame.gpu->getActiveShader()->setUniformIVec2("u_sourceResolution", glm::ivec2{viewport.x / pow(2.0f, i), viewport.y / pow(2.0f, i)});

            glBindImageTexture(0, resources.bloomTexture.getID(), i, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, resources.bloomTexture.getID(), i + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            glDispatchCompute((mipSize.x + 15) / 16, (mipSize.y + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        frame.gpu->changeActiveShader("bloomUpSample");

        for (int i = resources.bloomIterations - 2; i >= 0; --i) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, resources.bloomTexture.getID());
            frame.gpu->getActiveShader()->setUniformInt("u_lowerMipLevel", i + 1);
            glBindImageTexture(1, resources.bloomTexture.getID(), i, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

            mipSize = glm::ivec2(viewport.x, viewport.y) / (1 << i);
            glDispatchCompute((mipSize.x + 15) / 16, (mipSize.y + 15) / 16, 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resources.bloomTexture.getID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, resources.bloomIterations - 1);
    }
}
