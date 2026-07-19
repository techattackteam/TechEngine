#include "AutoExposurePass.hpp"

#include <algorithm>
#include <cmath>

#include "core/Logger.hpp"
#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/resources/GpuResourceManager.hpp"

namespace TechEngine {
    void AutoExposurePass::execute(const FrameContext& frame, RenderResources& resources) {
        const glm::ivec2 viewport = frame.viewportSize;

        frame.gpu->changeActiveShader("histogram");
        FrameBuffer& hdrFBO = resources.getFramebuffer(resources.gBufferFBO);
        uint32_t zero = 0;

        resources.histogramBuffer.bind();
        resources.histogramBuffer.setBindingPoint(0);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindImageTexture(0, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT3), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);


        uint32_t numGroupsX = (viewport.x + 15) / 16;
        uint32_t numGroupsY = (viewport.y + 15) / 16;
        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        uint32_t* histogramData = resources.histogramBuffer.mapBuffer(GL_READ_ONLY);
        constexpr int HISTOGRAM_BINS = 256;

        if (histogramData) {
            uint32_t pixelCount = viewport.x * viewport.y;
            const float lowPercent = 0.50f; // 40% of pixels can be darker
            const float highPercent = 0.90f; // 95% of pixels should be darker (i.e., ignore top 5% brightest)

            uint32_t minPixelCount = (uint32_t)(pixelCount * lowPercent);
            uint32_t maxPixelCount = (uint32_t)(pixelCount * highPercent);

            uint32_t currentPixels = 0;
            uint32_t minBin = 0;
            uint32_t maxBin = 255;

            for (int i = 0; i < HISTOGRAM_BINS; i++) {
                currentPixels += histogramData[i];
                if (currentPixels >= maxPixelCount) {
                    maxBin = i;
                    break;
                }
            }

            for (int i = 0; i < 256; ++i) {
                currentPixels += histogramData[i];
                if (currentPixels >= minPixelCount) {
                    minBin = i;
                    break;
                }
            }

            float minLogLum = -10.0f;
            float maxLogLum = 3.0f;
            float avgLogLuminance = 0;
            uint32_t samples = 0;

            for (uint32_t i = minBin; i <= maxBin; ++i) {
                if (histogramData[i] > 0) {
                    float binLuminance = minLogLum + (i / 255.0f) * (maxLogLum - minLogLum);
                    avgLogLuminance += binLuminance * histogramData[i];
                    samples += histogramData[i];
                }
            }

            if (samples > 0) {
                avgLogLuminance /= samples;
            }

            float avgLuminance = exp(avgLogLuminance);

            float keyValue = 0.18f;
            m_targetExposure = keyValue / avgLuminance;
            m_targetExposure = std::max(0.25f, std::min(m_targetExposure, 2.0f));
            TE_LOGGER_INFO("Average Luminance: {0}, Average Log Luminance {1}, Target Exposure {2}", avgLuminance, avgLogLuminance, m_targetExposure);
        }
        m_currentExposure = m_currentExposure + (m_targetExposure - m_currentExposure);
        if (frame.currentExposure) {
            *frame.currentExposure = m_currentExposure;
        }
        TE_LOGGER_INFO("Current Exposure: {0}", m_currentExposure);
        resources.histogramBuffer.unmapBuffer();
        resources.histogramBuffer.unBind();
    }
}
