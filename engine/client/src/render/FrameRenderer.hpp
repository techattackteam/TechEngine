#pragma once

namespace TechEngine {
    struct FrameCommand;
    struct FramebufferSize;

    class FrameRenderer {
    private:
        unsigned int m_triangleVao = 0;
        unsigned int m_triangleVbo = 0;
        unsigned int m_vertexShader = 0;
        unsigned int m_fragmentShader = 0;
        unsigned int m_shaderProgram = 0;

    public:
        FrameRenderer() = default;

        FrameRenderer(const FrameRenderer&) = delete;

        FrameRenderer& operator=(const FrameRenderer&) = delete;

        FrameRenderer(FrameRenderer&&) = delete;

        FrameRenderer& operator=(FrameRenderer&&) = delete;

        bool initialize();

        void draw(const FrameCommand& command, const FramebufferSize& size);

        void shutdown();
    };
}
