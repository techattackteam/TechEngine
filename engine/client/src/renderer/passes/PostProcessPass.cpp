#include "PostProcessPass.hpp"

#include "renderer/GLFW.hpp"
#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderSettings.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "renderer/shaders/Shader.hpp"

namespace TechEngine {
    bool PostProcessPass::isEnabled(const FrameContext& frame) const {
        return frame.renderMask & RenderMasks::POST_PROCESS;
    }

    void PostProcessPass::execute(const FrameContext& frame, RenderResources& resources) {
        // Composite into the request's target framebuffer (what the editor/runtime displays).
        FrameBuffer& target = resources.getFramebuffer(frame.targetFramebufferId);
        target.bind();
        target.resize(frame.viewportSize.x, frame.viewportSize.y);
        target.clear();

        const RenderSettings& settings = *frame.settings;

        glDisable(GL_DEPTH_TEST);

        frame.gpu->changeActiveShader("postProcess");
        Shader* postProcessShader = frame.gpu->getActiveShader();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resources.getFramebuffer(resources.gBufferFBO).getTextureID(GL_COLOR_ATTACHMENT3));
        postProcessShader->setUniformInt("u_hdrBuffer", 0);


        postProcessShader->setUniformBool("u_bloomEnabled", settings.bloom.enabled);
        postProcessShader->setUniformFloat("u_bloomStrength", settings.bloom.intensity);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, resources.bloomTexture.getID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, resources.bloomIterations - 1);
        postProcessShader->setUniformInt("u_bloomBuffer", 1);

        // Lift-Gamma-Gain
        postProcessShader->setUniformVec3("u_lift", settings.gamma.lift);
        postProcessShader->setUniformFloat("u_liftIntensity", settings.gamma.liftIntensity);
        postProcessShader->setUniformFloat("u_gamma", settings.gamma.gamma);
        postProcessShader->setUniformVec3("u_gammaRGB", settings.gamma.gammaRGB);
        postProcessShader->setUniformFloat("u_gammaIntensity", settings.gamma.gammaIntensity);
        postProcessShader->setUniformVec3("u_gain", settings.gamma.gain);
        postProcessShader->setUniformFloat("u_gainIntensity", settings.gamma.gainIntensity);

        // Color grading adjustments
        postProcessShader->setUniformFloat("u_exposure", settings.colorGrading.exposure);
        postProcessShader->setUniformFloat("u_saturation", settings.colorGrading.saturation);
        postProcessShader->setUniformFloat("u_contrast", settings.colorGrading.contrast);
        postProcessShader->setUniformFloat("u_brightness", settings.colorGrading.brightness);
        if (settings.colorGrading.useLUT) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_3D, resources.colorGradingLUT.getID());
            postProcessShader->setUniformInt("u_colorGradingLUT", 2);
        }
        postProcessShader->setUniformBool("u_useLUT", settings.colorGrading.useLUT);
        postProcessShader->setUniformFloat("u_lutStrength", settings.colorGrading.lutStrength);
        postProcessShader->setUniformInt("u_lutSize", 32); // or make this configurable

        // Chromatic Aberration
        postProcessShader->setUniformBool("u_chromaticAberrationEnabled", settings.chromaticAberration.enabled);
        postProcessShader->setUniformFloat("u_chromaticAberrationStrength", settings.chromaticAberration.strength);
        postProcessShader->setUniformFloat("u_chromaticAberrationOffset", settings.chromaticAberration.offset);

        // Vignette
        postProcessShader->setUniformBool("u_vignetteEnabled", settings.vignette.enabled);
        postProcessShader->setUniformFloat("u_vignetteStrength", settings.vignette.strength);
        postProcessShader->setUniformFloat("u_vignettePower", settings.vignette.power);

        // Film Grain
        postProcessShader->setUniformBool("u_filmGrainEnabled", settings.filmGrain.filmGrainEnabled);
        postProcessShader->setUniformFloat("u_filmGrainIntensity", settings.filmGrain.filmGrainIntensity);
        postProcessShader->setUniformFloat("u_filmGrainSize", settings.filmGrain.filmGrainSize);
        postProcessShader->setUniformFloat("u_time", glfwGetTime()); // or your timer


        resources.vertexArrays[RenderResources::BufferFullscreenQuad].bind();
        glDrawArrays(GL_TRIANGLES, 0, 6);
        resources.vertexArrays[RenderResources::BufferFullscreenQuad].unBind();

        glEnable(GL_DEPTH_TEST);

        target.unBind();
    }
}
