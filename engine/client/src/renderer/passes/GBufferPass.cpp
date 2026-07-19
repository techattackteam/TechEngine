#include "GBufferPass.hpp"

#include "renderer/graph/FrameContext.hpp"
#include "renderer/graph/RenderResources.hpp"
#include "renderer/resources/GpuResourceManager.hpp"

namespace TechEngine {
    bool GBufferPass::isEnabled(const FrameContext& frame) const {
        return frame.renderMask & RenderMasks::SCENE;
    }

    void GBufferPass::prepare(const FrameContext& frame, RenderResources& resources) {
        const glm::ivec2 viewport = frame.viewportSize;
        FrameBuffer& frameBuffer = resources.getFramebuffer(resources.gBufferFBO);
        frameBuffer.bind();
        frameBuffer.resize(viewport.x, viewport.y);
        glViewport(0, 0, viewport.x, viewport.y);
        glClearColor(0.0f, 0.0f, 0.0, 0.0f);

        GLenum attachments[] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3,
        };

        glDrawBuffers(4, attachments);

        frameBuffer.clear();
        frameBuffer.unBind();
    }

    void GBufferPass::execute(const FrameContext& frame, RenderResources& resources) {
        prepare(frame, resources);

        const glm::ivec2 viewport = frame.viewportSize;
        FrameBuffer& frameBuffer = resources.getFramebuffer(resources.gBufferFBO);
        frameBuffer.bind();
        glViewport(0, 0, viewport.x, viewport.y);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glCullFace(GL_BACK);

        frame.gpu->changeActiveShader("gBuffer");
        frame.gpu->getActiveShader()->setUniformMatrix4f("u_projection", frame.projectionMatrix);
        frame.gpu->getActiveShader()->setUniformMatrix4f("u_view", frame.viewMatrix);

        GLenum attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

        glDrawBuffers(2, attachments);

        resources.drawCommandBuffer.setBindingPoint(0);
        resources.objectDataBuffer.setBindingPoint(1);
        frame.gpu->m_materialsBuffer.setBindingPoint(2);
        resources.objectDataBuffer.bind();
        frame.gpu->m_meshesVertexArray.bind();
        frame.gpu->m_meshesIndexBuffer.bind();
        resources.drawCommandBuffer.bind();
        frame.gpu->m_materialsBuffer.bind();

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
