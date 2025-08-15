#include "UIRenderer.hpp"

#include "project/Project.hpp"
#include "renderer/ErrorCatcher.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    UIRenderer::UIRenderer(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    UIRenderer::~UIRenderer() {
    }

    void UIRenderer::init() {
        std::string clientPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string();
        m_uiShader = m_uiShader = new Shader("ui", (clientPath + "/assets/shaders/uiVertex.glsl").c_str(),
                                             (clientPath + "/assets/shaders/uiFragment.glsl").c_str());
        m_vertexArray = new VertexArray();
        m_vertexArray->init();

        m_vertexBuffer = new VertexBuffer();
        m_vertexBuffer->init(m_maxVertices * sizeof(UIVertex));

        m_vertexArray->addNewUIBuffer(*m_vertexBuffer);

        m_vertices.reserve(m_maxVertices);
    }

    void UIRenderer::onUpdate() {
        m_uiShader->bind();
        m_vertexArray->bind();
        m_vertexBuffer->bind();

        m_vertexBuffer->addData(m_vertices.data(), m_vertices.size() * sizeof(UIVertex), 0);

        m_uiShader->setUniformMatrix4f("projection", m_projection);
        m_uiShader->setUniformBool("uUseTexture", false);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        GlCall(glDrawArrays(GL_TRIANGLES, 0, m_vertices.size()));

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        m_vertices.clear();
        m_uiShader->unBind();
        m_vertexArray->unBind();
        m_vertexBuffer->unBind();
    }

    void UIRenderer::shutdown() {
        delete m_uiShader;
        m_uiShader = nullptr;

        delete m_vertexArray;
        m_vertexArray = nullptr;

        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
    }

    void UIRenderer::setViewport(int width, int height) {
        if (width == m_screenWidth && height == m_screenHeight) {
            return;
        }
        m_screenWidth = width;
        m_screenHeight = height;
        m_projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
    }

    void UIRenderer::drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
        UIVertex v0 = UIVertex(position, color, glm::vec2(0.0f, 0.0f), 0.0f);
        UIVertex v1 = UIVertex(position + glm::vec2(size.x, 0.0f), color, glm::vec2(1.0f, 0.0f), 0.0f);
        UIVertex v2 = UIVertex(position + glm::vec2(size.x, size.y), color, glm::vec2(1.0f, 1.0f), 0.0f);
        UIVertex v3 = UIVertex(position + glm::vec2(0.0f, size.y), color, glm::vec2(0.0f, 1.0f), 0.0f);

        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);

        m_vertices.push_back(v2);
        m_vertices.push_back(v3);
        m_vertices.push_back(v0);
    }
}
