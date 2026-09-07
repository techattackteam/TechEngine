#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/client/render/FrameCommand.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/FrameRenderer.hpp>

#include <glad/gl.h>

#include <array>

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
        const float vertices[] = {-0.5F, -0.5F, 0.0F, 0.5F, -0.5F, 0.0F, 0.0F, 0.5F, 0.0F};
        glGenBuffers(1, &m_triangleVbo);
        glGenVertexArrays(1, &m_triangleVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_triangleVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(m_triangleVao);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

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

    void FrameRenderer::draw(const FrameCommand& command, const FramebufferSize& size) {
        if (size.width == 0 || size.height == 0) {
            return;
        }
        glViewport(0, 0, size.width, size.height);
        glClearColor(command.clearColor[0], command.clearColor[1], command.clearColor[2], command.clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        if (command.drawTriangle) {
            glUseProgram(m_shaderProgram);
            glBindVertexArray(m_triangleVao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }
    }

    void FrameRenderer::shutdown() {
        if (m_triangleVbo != 0) {
            glDeleteBuffers(1, &m_triangleVbo);
            m_triangleVbo = 0;
        }
        if (m_triangleVao != 0) {
            glDeleteVertexArrays(1, &m_triangleVao);
            m_triangleVao = 0;
        }
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
