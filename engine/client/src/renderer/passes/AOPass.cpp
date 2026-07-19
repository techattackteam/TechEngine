#include "AOPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/graph/RenderSettings.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "renderer/shaders/Shader.hpp"

namespace TechEngine {
    void AOPass::execute(const FrameContext& frame, RenderResources& resources) {
        const glm::mat4 projectionMatrix = frame.projectionMatrix;
        const glm::ivec2 viewport = frame.viewportSize;
        const AOProperties& aoProperties = frame.settings->ao;

        static int frameCounter = 0;
        constexpr int NUM_ROTATIONS = 6;

        if (resources.aoTexture.getWidth() != viewport.x || resources.aoTexture.getHeight() != viewport.y) {
            resources.aoTexture.deleteTexture();
            resources.aoTexture.create(GL_TEXTURE_2D, GL_RGBA16F, viewport.x, viewport.y, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr);
        }

        frame.gpu->changeActiveShader("ao");
        Shader* aoHorizon = frame.gpu->getActiveShader();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resources.getFramebuffer(resources.gBufferFBO).getTextureID(GL_DEPTH_ATTACHMENT)); // Positions
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, resources.getFramebuffer(resources.gBufferFBO).getTextureID(GL_COLOR_ATTACHMENT1)); // Normals

        aoHorizon->setUniformBool("u_enabled", aoProperties.enabled);
        aoHorizon->setUniformFloat("u_directionCount", static_cast<float>(aoProperties.directionCount));
        aoHorizon->setUniformFloat("u_stepsPerDirection", static_cast<float>(aoProperties.stepsPerDirection));
        aoHorizon->setUniformFloat("u_radius", aoProperties.radius);
        aoHorizon->setUniformFloat("u_thickness", aoProperties.thickness);
        aoHorizon->setUniformVec2("u_screenSize", viewport);
        aoHorizon->setUniformMatrix4f("u_projection", projectionMatrix);
        aoHorizon->setUniformMatrix4f("u_inverseProjection", glm::inverse(projectionMatrix));

        glBindImageTexture(0, resources.aoTexture.getID(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        const uint32_t WORKGROUP_SIZE = 8;
        uint32_t numGroupsX = (viewport.x + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
        uint32_t numGroupsY = (viewport.y + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;

        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        frameCounter = (frameCounter + 1) % NUM_ROTATIONS;
    }
}
