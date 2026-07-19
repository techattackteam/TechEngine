#include "FogPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderSettings.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "renderer/shaders/Shader.hpp"
#include "scene/SceneManager.hpp"
#include "systems/SystemsRegistry.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    void FogPass::execute(const FrameContext& frame, RenderResources& resources) {
        const FogProperties& fogProperties = frame.settings->fog;

        resources.recreateFogTexture(frame.viewportSize);

        frame.gpu->changeActiveShader("atmosphericFog");
        Shader* fogShader = frame.gpu->getActiveShader();
        fogShader->bind();

        FrameBuffer& gBuffer = resources.getFramebuffer(resources.gBufferFBO);
        glBindImageTexture(0, gBuffer.getTextureID(GL_COLOR_ATTACHMENT3), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindImageTexture(1, resources.fogTexture.getID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.getTextureID(GL_DEPTH_ATTACHMENT));
        fogShader->setUniformInt("u_depthBuffer", 0);

        fogShader->setUniformFloat("u_fogDensity", fogProperties.fogDensity);
        fogShader->setUniformFloat("u_fogHeightFalloff", fogProperties.fogHeightFalloff);
        fogShader->setUniformFloat("u_fogHeight", fogProperties.fogHeight);
        fogShader->setUniformFloat("u_fogStart", fogProperties.fogStart);
        fogShader->setUniformFloat("u_fogEnd", fogProperties.fogEnd);

        fogShader->setUniformVec3("u_fogColorBase", fogProperties.fogColorBase);
        fogShader->setUniformVec3("u_fogColorSky", fogProperties.fogColorSky);
        fogShader->setUniformVec3("u_fogColorSun", fogProperties.fogColorSun);
        fogShader->setUniformInt("u_fogBlendMode", fogProperties.fogBlendMode);
        fogShader->setUniformBool("u_useDirectionalColor", fogProperties.useDirectionalColor);
        fogShader->setUniformFloat("u_sunScatteringIntensity", fogProperties.sunScatteringIntensity);

        fogShader->setUniformFloat("u_mieScattering", fogProperties.mieScattering);
        fogShader->setUniformFloat("u_rayleighScattering", fogProperties.rayleighScattering);
        fogShader->setUniformFloat("u_skyboxFogAmount", fogProperties.skyboxFogAmount);
        // Camera data
        fogShader->setUniformVec3("u_cameraPosition", glm::inverse(frame.viewMatrix)[3]);
        glm::mat4 inverseProjection = glm::inverse(frame.projectionMatrix * frame.viewMatrix);
        fogShader->setUniformMatrix4f("u_inverseViewProjection", inverseProjection);

        Scene& scene = frame.systems->getSystem<SceneManager>().getActiveScene();
        glm::vec3 sunDir = glm::vec3(0.0f, -1.0f, 0.0f);
        scene.runSystem<Transform, DirectionalLight>([&](Transform& transform, DirectionalLight& light) {
            glm::mat3 rotationMatrix = glm::mat3(transform.getModelMatrix());
            glm::vec3 modelForward = glm::vec3(0.0f, 0.0f, -1.0f);
            sunDir = glm::normalize(rotationMatrix * modelForward);
        });
        fogShader->setUniformVec3("u_sunDirection", sunDir);

        uint32_t numGroupsX = (frame.viewportSize.x + 7) / 8;
        uint32_t numGroupsY = (frame.viewportSize.y + 7) / 8;
        glDispatchCompute(numGroupsX, numGroupsY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glCopyImageSubData(
            resources.fogTexture.getID(), GL_TEXTURE_2D, 0, 0, 0, 0,
            gBuffer.getTextureID(GL_COLOR_ATTACHMENT3), GL_TEXTURE_2D, 0, 0, 0, 0,
            frame.viewportSize.x, frame.viewportSize.y, 1
        );
    }
}
