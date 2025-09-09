#pragma once
#include "core/ExportDLL.hpp"
#include "UIVertex.hpp"
#include "renderer/VertexArray.hpp"
#include "renderer/Shader.hpp"

#include "glm/glm.hpp"

namespace TechEngine {
    class SystemsRegistry;
    class Font;

    struct UIDrawCommand {
        uint32_t textureID = 0; // Texture ID for the UI element, 0 if no texture
        uint32_t vertexCount = 0;
        uint32_t vertexOffset = 0;
    };

    class CLIENT_DLL UIRenderer {
    private:
        SystemsRegistry& m_systemsRegistry;
        bool m_initialized = false;

        Shader* m_uiShader;
        VertexArray* m_vertexArray;
        VertexBuffer* m_vertexBuffer;

        std::vector<UIVertex> m_vertices;
        std::vector<UIDrawCommand> m_drawCommands;
        uint32_t m_currentTextureID = 0;

        uint32_t m_maxVertices = 30000; // Ideally is infinit but we limit it for performance
        glm::mat4 m_projection;
        float m_dpiScale = 0.0f;

    public:
        int m_screenWidth = 0;
        int m_screenHeight = 0;
        Font* m_defaultFont = nullptr;

        UIRenderer(SystemsRegistry& systemsRegistry);

        ~UIRenderer();

        void init();

        void beginFrame();

        void endFrame();

        void shutdown();

        void setViewport(int width, int height);

        void drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

        void drawText(Font* font, const std::string& text, const glm::vec2& position, float fontSize, const glm::vec4& color);

        const float getDpiScale();

    private:
        void flushCommands();
    };
}
