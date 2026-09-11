#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/client/render/RenderSnapshot.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/FrameRenderer.hpp>

#include <glad/gl.h>

#include <array>
#include <cstdint>
#include <span>

namespace TechEngine {
    static bool compileShader(unsigned int shader, const char* source, const char* stage) {
        if (shader == 0) {
            TE_LOGGER_ERROR("Failed to create {0} shader", stage);
            return false;
        }
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        int compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
            return true;
        }
        std::array<char, 4096> log{};
        glGetShaderInfoLog(shader, static_cast<int>(log.size()), nullptr, log.data());
        TE_LOGGER_ERROR("Failed to compile {0} shader: {1}", stage, log.data());
        return false;
    }

    bool FrameRenderer::initialize() {
        const std::array<float, 9> vertices{-0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F, 0.0F, 0.5F, 0.0F};
        const std::array<std::uint32_t, 3> indices{0, 1, 2};
        if (!m_triangleVertexBuffer.initialize(std::as_bytes(std::span{vertices})) || !m_triangleIndexBuffer.initialize(std::as_bytes(std::span{indices})) || !m_triangleVertexArray.initialize()) {
            shutdown();
            return false;
        }

        constexpr std::uint32_t VERTEX_BINDING = 0;
        m_triangleVertexArray.setVertexBuffer(m_triangleVertexBuffer, VERTEX_BINDING, 0, static_cast<int>(3 * sizeof(float)));
        m_triangleVertexArray.setIndexBuffer(m_triangleIndexBuffer);
        m_triangleVertexArray.setFloatAttribute(0, VERTEX_BINDING, 3, 0);

        m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
        m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        const char* vertexShaderSource = R"(
            #version 450 core
            layout(location = 0) in vec3 aPos;
            void main() {
                gl_Position = vec4(aPos, 1.0);
            }
        )";
        const char* fragmentShaderSource = R"(
            #version 450 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.5, 0.2, 1.0);
            }
        )";

        if (!compileShader(m_vertexShader, vertexShaderSource, "vertex") || !compileShader(m_fragmentShader, fragmentShaderSource, "fragment")) {
            shutdown();
            return false;
        }

        m_shaderProgram = glCreateProgram();
        if (m_shaderProgram == 0) {
            TE_LOGGER_ERROR("Failed to create triangle shader program");
            shutdown();
            return false;
        }
        glAttachShader(m_shaderProgram, m_vertexShader);
        glAttachShader(m_shaderProgram, m_fragmentShader);
        glLinkProgram(m_shaderProgram);
        int linked = GL_FALSE;
        glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            std::array<char, 4096> log{};
            glGetProgramInfoLog(m_shaderProgram, static_cast<int>(log.size()), nullptr, log.data());
            TE_LOGGER_ERROR("Failed to link triangle shader program: {0}", log.data());
            shutdown();
            return false;
        }
        return true;
    }

    void FrameRenderer::draw(const RenderSnapshot& command, const FramebufferSize& size) {
        if (size.width == 0 || size.height == 0) {
            return;
        }
        glViewport(0, 0, size.width, size.height);
        glClearColor(command.clearColor[0], command.clearColor[1], command.clearColor[2], command.clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        if (command.drawTriangle) {
            glUseProgram(m_shaderProgram);
            m_triangleVertexArray.bind();
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
            VertexArray::unbind();
        }
    }

    void FrameRenderer::shutdown() {
        m_triangleVertexArray.shutdown();
        m_triangleIndexBuffer.shutdown();
        m_triangleVertexBuffer.shutdown();
        if (m_vertexShader != 0) {
            glDeleteShader(m_vertexShader);
            m_vertexShader = 0;
        }
        if (m_fragmentShader != 0) {
            glDeleteShader(m_fragmentShader);
            m_fragmentShader = 0;
        }
        if (m_shaderProgram != 0) {
            glUseProgram(0);
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
        }
    }
}
