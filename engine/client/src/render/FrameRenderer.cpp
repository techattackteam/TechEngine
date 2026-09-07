#include <TechEngine/client/render/FrameCommand.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include "glad/gl.h"
#include <render/FrameRenderer.hpp>

namespace TechEngine {
    bool FrameRenderer::initialize() {
        // TODO(S5-T8): create triangle GL resources, cleaning up on failure.
        float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};
        glGenBuffers(1, &m_triangleVbo);
        glGenVertexArrays(1, &m_triangleVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_triangleVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(m_triangleVao);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return true;
    }

    void FrameRenderer::draw(const FrameCommand& command, const FramebufferSize& size) {
        if (size.width == 0 || size.height == 0) {
            return;
        }
        glViewport(0, 0, size.width, size.height);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(m_triangleVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        (void)command;
    }

    void FrameRenderer::shutdown() {
        // TODO(S5-T8): delete GL resources before releasing the context; allow repeated shutdown.
        if (m_triangleVbo != 0) {
            glDeleteBuffers(1, &m_triangleVbo);
            m_triangleVbo = 0;
        }
        if (m_triangleVao != 0) {
            glDeleteVertexArrays(1, &m_triangleVao);
            m_triangleVao = 0;
        }
    }
}
