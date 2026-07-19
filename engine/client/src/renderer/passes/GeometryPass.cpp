#include "GeometryPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/resources/GpuResourceManager.hpp"
#include "renderer/texture/SkyBox.hpp"

namespace TechEngine {
    bool GeometryPass::isEnabled(const FrameContext& frame) const {
        return frame.renderMask & RenderMasks::SCENE;
    }

    void GeometryPass::execute(const FrameContext& frame, RenderResources& resources) {
        const glm::mat4 viewMatrix = frame.viewMatrix;
        const glm::mat4 projectionMatrix = frame.projectionMatrix;
        const glm::ivec2 viewport = frame.viewportSize;
        const float nearPlane = frame.nearPlane;
        const float farPlane = frame.farPlane;

        FrameBuffer& frameBuffer = resources.getFramebuffer(resources.gBufferFBO);
        frameBuffer.bind();
        frameBuffer.clear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, viewport.x, viewport.y);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        frame.gpu->changeActiveShader("geometry");
        frame.gpu->getActiveShader()->setUniformMatrix4f("u_projection", projectionMatrix);
        frame.gpu->getActiveShader()->setUniformMatrix4f("u_view", viewMatrix);
        glm::vec3 cameraPosition = glm::inverse(viewMatrix)[3];

        frame.gpu->getActiveShader()->setUniformVec3("u_cameraPos", cameraPosition);
        frame.gpu->getActiveShader()->setUniformIVec2("u_screenSize", viewport);
        frame.gpu->getActiveShader()->setUniformFloat("u_nearPlane", nearPlane);
        frame.gpu->getActiveShader()->setUniformFloat("u_farPlane", farPlane);
        frame.gpu->getActiveShader()->setUniformBool("u_debugLightCulling", frame.debugLightCulling);
        float scale = (float)frame.gridSize.z / std::log2f(farPlane / nearPlane);
        float bias = -((float)frame.gridSize.z * std::log2f(nearPlane) / std::log2f(farPlane / nearPlane));
        frame.gpu->getActiveShader()->setUniformFloat("u_depthSliceScale", scale);
        frame.gpu->getActiveShader()->setUniformFloat("u_depthSliceBias", bias);
        frame.gpu->getActiveShader()->setUniformUVec3("u_gridSize", frame.gridSize);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, frame.skyBox->m_irradianceMap.getID());
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, frame.skyBox->m_prefilterMap.getID());
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, frame.skyBox->m_brdfLUTTexture.getID());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, resources.aoTexture.getID());

        frame.gpu->getActiveShader()->setUniformInt("u_irradianceMap", 3);
        frame.gpu->getActiveShader()->setUniformInt("u_prefilterMap", 1);
        frame.gpu->getActiveShader()->setUniformInt("u_brdfLUT", 2);
        frame.gpu->getActiveShader()->setUniformInt("u_aoMap", 0);

        GLenum attachments[2] = {GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT2};

        glDrawBuffers(2, attachments);

        resources.drawCommandBuffer.setBindingPoint(0);
        resources.objectDataBuffer.setBindingPoint(1);
        frame.gpu->m_materialsBuffer.setBindingPoint(2);
        resources.lightsBuffer.setBindingPoint(3);
        resources.lightsIndexBuffer.setBindingPoint(4);
        resources.tileInfoBuffer.setBindingPoint(5);

        resources.objectDataBuffer.bind();
        frame.gpu->m_meshesVertexArray.bind();
        frame.gpu->m_meshesIndexBuffer.bind();
        resources.drawCommandBuffer.bind();

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resources.drawCommandBuffer.getID());

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            (void*)0,
            frame.commandToDraw,
            0
        );

        resources.objectDataBuffer.unBind();
        frame.gpu->m_meshesVertexArray.unBind();
        frame.gpu->m_meshesIndexBuffer.unBind();
        resources.drawCommandBuffer.unBind();
        frameBuffer.unBind();
    }
}
