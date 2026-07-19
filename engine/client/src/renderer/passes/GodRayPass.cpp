#include "GodRayPass.hpp"

#include <cmath>

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderSettings.hpp"
#include "renderer/graph/RenderTypes.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    void GodRayPass::execute(const FrameContext& frame, RenderResources& resources) {
        FroxelParams& froxelParams = frame.settings->froxelParams;
        FroxelGridProperties& froxelGrid = frame.settings->froxelGrid;
        VolumetricSettings& volumetricSettings = frame.settings->volumetric;

        glm::vec3 cameraPosition = glm::inverse(frame.viewMatrix)[3];
        populateLightsBuffer(*frame.systems, resources.lightsBuffer);
        froxelGrid.nearPlane = frame.nearPlane;
        froxelGrid.farPlane = frame.farPlane;

        froxelParams.viewMatrix = frame.viewMatrix;
        froxelParams.viewProjectionInverse = glm::inverse(frame.projectionMatrix * frame.viewMatrix);

        froxelParams.froxelDimensions = glm::uvec3(froxelGrid.width, froxelGrid.height, froxelGrid.depth);

        froxelParams.froxelNearPlane = froxelGrid.nearPlane;
        froxelParams.froxelFarPlane = froxelGrid.farPlane;
        froxelParams.depthDistributionScale = froxelGrid.depthDistributionScale;
        froxelParams.useExponentialDepth = froxelGrid.useExponentialDepth ? 1 : 0;

        froxelParams.rcpFroxelDimX = 1.0f / static_cast<float>(froxelGrid.width);
        froxelParams.rcpFroxelDimY = 1.0f / static_cast<float>(froxelGrid.height);
        froxelParams.rcpFroxelDimZ = 1.0f / static_cast<float>(froxelGrid.depth);
        froxelParams.cameraPosition = cameraPosition;

        if (froxelParams.myImplementation) {
            glBindBuffer(GL_UNIFORM_BUFFER, resources.froxelParamsUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FroxelParams), &froxelParams);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glBindBuffer(GL_UNIFORM_BUFFER, resources.volumetricSettingsUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(VolumetricSettings), &volumetricSettings);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            GLfloat clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            glClearTexImage(resources.froxelTexture.getID(), 0, GL_RGBA, GL_FLOAT, clearColor);
            glClearTexImage(resources.volumetricLightVolume.getID(), 0, GL_RGBA, GL_FLOAT, clearColor);

            // Use compute shader
            frame.gpu->changeActiveShader("froxelScattering");
            // Bind UBO
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, resources.froxelParamsUBO);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, resources.volumetricSettingsUBO);
            resources.lightsBuffer.setBindingPoint(0);

            // Bind 3D texture as image
            glBindImageTexture(0, resources.froxelTexture.getID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            // Dispatch compute
            uint32_t groupsX = (froxelGrid.width + 7) / 8;
            uint32_t groupsY = (froxelGrid.height + 7) / 8;
            uint32_t groupsZ = froxelGrid.depth;

            glDispatchCompute(groupsX, groupsY, groupsZ);

            // Memory barrier
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glUseProgram(0);

            frame.gpu->changeActiveShader("volumetricRayMarching");
            Shader* volumetricShader = frame.gpu->getActiveShader();
            FrameBuffer& hdrFBO = resources.getFramebuffer(resources.gBufferFBO);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, resources.froxelTexture.getID());
            volumetricShader->setUniformInt("u_froxelScattering", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, hdrFBO.getTextureID(GL_DEPTH_ATTACHMENT));
            volumetricShader->setUniformInt("u_depthBuffer", 1);

            volumetricShader->setUniformFloat("u_blendingFactor", volumetricSettings.blendingFactor);


            glBindImageTexture(0, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT3), 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

            glBindBufferBase(GL_UNIFORM_BUFFER, 0, resources.froxelParamsUBO);

            groupsX = (frame.viewportSize.x + 7) / 8;
            groupsY = (frame.viewportSize.y + 7) / 8;
            groupsZ = 1;

            glDispatchCompute(groupsX, groupsY, groupsZ);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                            GL_TEXTURE_FETCH_BARRIER_BIT |
                            GL_FRAMEBUFFER_BARRIER_BIT);

            glUseProgram(0);
        } else {
            return; // Temporary disable other implementation
            glBindBuffer(GL_UNIFORM_BUFFER, resources.froxelParamsUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(FroxelParams), &froxelParams);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            glBindBuffer(GL_UNIFORM_BUFFER, resources.volumetricSettingsUBO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(VolumetricSettings), &volumetricSettings);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);

            GLfloat clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            glClearTexImage(resources.froxelTexture.getID(), 0, GL_RGBA, GL_FLOAT, clearColor);
            glClearTexImage(resources.volumetricLightVolume.getID(), 0, GL_RGBA, GL_FLOAT, clearColor);

            // Use compute shader
            frame.gpu->changeActiveShader("VolFroxelScatteringOtherImpCompute");
            // Bind UBO
            glBindBufferBase(GL_UNIFORM_BUFFER, 0, resources.froxelParamsUBO);
            glBindBufferBase(GL_UNIFORM_BUFFER, 1, resources.volumetricSettingsUBO);
            resources.lightsBuffer.setBindingPoint(0);

            // Bind 3D texture as image
            glBindImageTexture(0, resources.froxelTexture.getID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            // Dispatch compute
            uint32_t groupsX = (froxelGrid.width + 7) / 8;
            uint32_t groupsY = (froxelGrid.height + 7) / 8;
            uint32_t groupsZ = froxelGrid.depth;

            glDispatchCompute(groupsX, groupsY, groupsZ);

            // Memory barrier
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glUseProgram(0);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            glUseProgram(0);

            frame.gpu->changeActiveShader("VolRayMarchinOtherImpCompute");
            Shader* volumetricShader = frame.gpu->getActiveShader();
            FrameBuffer& hdrFBO = resources.getFramebuffer(resources.gBufferFBO);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_3D, resources.froxelTexture.getID());
            volumetricShader->setUniformInt("s_VoxelGrid", 0);

            glBindImageTexture(0, resources.volumetricLightVolume.getID(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

            glBindBufferBase(GL_UNIFORM_BUFFER, 0, resources.froxelParamsUBO);

            uint32_t size_x = static_cast<uint32_t>(ceil(float(froxelParams.froxelDimensions.x) / float(8)));
            uint32_t size_y = static_cast<uint32_t>(ceil(float(froxelParams.froxelDimensions.y) / float(8)));
            uint32_t size_z = 1;
            glDispatchCompute(size_x, size_y, size_z);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                            GL_TEXTURE_FETCH_BARRIER_BIT |
                            GL_FRAMEBUFFER_BARRIER_BIT);

            glUseProgram(0);
            frame.gpu->changeActiveShader("sampleVolumetricLight");
            Shader* sampleVolumetricShader = frame.gpu->getActiveShader();

            sampleVolumetricShader->setUniformInt("u_hdrBuffer", 0);
            glBindImageTexture(0, hdrFBO.getTextureID(GL_COLOR_ATTACHMENT3), 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_3D, resources.volumetricLightVolume.getID());
            sampleVolumetricShader->setUniformInt("u_volumetricLightingVolume", 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, hdrFBO.getTextureID(GL_DEPTH_ATTACHMENT));
            sampleVolumetricShader->setUniformInt("u_depthBuffer", 2);

            sampleVolumetricShader->setUniformMatrix4f("u_viewProjection", frame.projectionMatrix * frame.viewMatrix);
            sampleVolumetricShader->setUniformFloat("u_blendingFactor", volumetricSettings.blendingFactor);
            groupsX = (frame.viewportSize.x + 7) / 8;
            groupsY = (frame.viewportSize.y + 7) / 8;
            groupsZ = 1;

            glDispatchCompute(groupsX, groupsY, groupsZ);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                            GL_TEXTURE_FETCH_BARRIER_BIT |
                            GL_FRAMEBUFFER_BARRIER_BIT);

            glUseProgram(0);
        }
    }
}
