#pragma once
#include "core/ExportDLL.hpp"
#include "UIVertex.hpp"
#include "renderer/VertexArray.hpp"
#include "renderer/Shader.hpp"

#include "glm/glm.hpp"

namespace TechEngine {
    class SystemsRegistry;


    class CLIENT_DLL UIRenderer {
    private:
        SystemsRegistry& m_systemsRegistry;
        bool m_initialized = false;

        Shader* m_uiShader;
        VertexArray* m_vertexArray;
        VertexBuffer* m_vertexBuffer;

        std::vector<UIVertex> m_vertices;
        uint32_t m_maxVertices = 30000; // Ideally is infinit but we limit it for performance

        glm::mat4 m_projection;
        int m_screenWidth = 0;
        int m_screenHeight = 0;

    public:
        UIRenderer(SystemsRegistry& systemsRegistry);

        ~UIRenderer();

        void init();

        void onUpdate();

        void shutdown();

        void setViewport(int width, int height);

        void drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    };
}
