#include "UIRenderer.hpp"

#include "Font.hpp"
#include "components/Archetype.hpp"
#include "project/Project.hpp"
#include "renderer/ErrorCatcher.hpp"
#include "renderer/VertexBuffer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "ui/WidgetsRegistry.hpp"

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
        m_defaultFont = new Font();
        if (!m_defaultFont->load("C:/Windows/Fonts/arial.ttf", 24)) {
            TE_LOGGER_ERROR("Failed to load default font");
        }
    }

    void UIRenderer::beginFrame() {
        m_vertices.clear();
        m_drawCommands.clear();
        m_currentTextureID = 0;
        m_drawCommands.push_back({0, 0, 0});
    }

    void UIRenderer::endFrame() {
        if (m_drawCommands.empty() || m_drawCommands.back().vertexCount == 0) {
            if (!m_drawCommands.empty()) {
                m_drawCommands.pop_back();
            }
        }

        if (m_vertices.empty()) return;

        m_uiShader->bind();
        m_vertexArray->bind();
        m_vertexBuffer->bind();

        m_vertexBuffer->addData(m_vertices.data(), m_vertices.size() * sizeof(UIVertex), 0);

        m_uiShader->setUniformMatrix4f("projection", m_projection);
        m_uiShader->setUniformBool("uUseTexture", false);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        for (const auto& drawCommand: m_drawCommands) {
            if (drawCommand.vertexCount == 0) {
                continue;
            } else if (drawCommand.textureID != 0) {
                m_uiShader->setUniformBool("uUseTexture", true);
                glBindTexture(GL_TEXTURE_2D, drawCommand.textureID);
            } else {
                m_uiShader->setUniformBool("uUseTexture", false);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            glDrawArrays(GL_TRIANGLES, drawCommand.vertexOffset, drawCommand.vertexCount);
        }


        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
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
        glm::vec2 referenceResolution = {1920.0f, 1080.0f}; // TODO: Make this configurable
        float m_matchMode = 0.5f;
        if (width == m_screenWidth && height == m_screenHeight) {
            return;
        }
        m_screenWidth = width;
        m_screenHeight = height;
        m_projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);

        float widthScale = width / referenceResolution.x;
        float heightScale = height / referenceResolution.y;
        float logWidth = log(widthScale);
        float logHeight = log(heightScale);
        float blend = logWidth * (1.0f - m_matchMode) + logHeight * m_matchMode; // lerp

        m_dpiScale = exp(blend);
        for (auto& widget: m_systemsRegistry.getSystem<WidgetsRegistry>().getWidgets()) {
            if (widget && widget->m_parent == nullptr) {
                widget->calculateLayout(glm::vec4(0.0f, 0.0f, (float)m_screenWidth, (float)m_screenHeight), m_dpiScale);
            }
        }
    }

    void UIRenderer::drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
        if (m_currentTextureID != 0) {
            if (m_drawCommands.back().vertexCount > 0) {
                m_drawCommands.push_back({0, 0, static_cast<uint32_t>(m_vertices.size())});
            } else {
                m_drawCommands.back().textureID = 0;
            }
            m_currentTextureID = 0;
        }
        UIVertex v0, v1, v2, v3;

        v0.position = position;
        v1.position = position + glm::vec2(size.x, 0.0f);
        v2.position = position + glm::vec2(size.x, size.y);
        v3.position = position + glm::vec2(0.0f, size.y);

        v0.texCoords = {0.0f, 0.0f};
        v1.texCoords = {1.0f, 0.0f};
        v2.texCoords = {1.0f, 1.0f};
        v3.texCoords = {0.0f, 1.0f};

        v0.color = v1.color = v2.color = v3.color = color;

        m_vertices.push_back(v0);
        m_vertices.push_back(v1);
        m_vertices.push_back(v2);

        m_vertices.push_back(v2);
        m_vertices.push_back(v3);
        m_vertices.push_back(v0);

        m_drawCommands.back().vertexCount += 6;
    }

    void UIRenderer::drawText(Font* font, const std::string& text, const glm::vec2& position, float fontSize, const glm::vec4& color) {
        if (!font || text.empty() || font->getAtlasTextureID() == (uint32_t)-1) {
            return;
        }

        const float nativeSize = font->m_nativeFontSize;
        const float scale = fontSize / nativeSize;

        uint32_t textureID = font->getAtlasTextureID();
        if (m_currentTextureID != textureID) {
            if (!m_drawCommands.empty() && m_drawCommands.back().vertexCount > 0) {
                m_drawCommands.push_back({textureID, 0, (uint32_t)m_vertices.size()});
            } else if (!m_drawCommands.empty()) {
                m_drawCommands.back().textureID = textureID;
            } else {
                m_drawCommands.push_back({textureID, 0, (uint32_t)m_vertices.size()});
            }
            m_currentTextureID = textureID;
        }

        float cursorX = position.x;
        float cursorY = position.y;
        for (char c: text) {
            if (c == '\n') {
                cursorY += font->getLineHeight() * scale;
                cursorX = position.x;
                continue;
            }
            CharInfo info;
            if (font->getCharInfo(c, info)) {
                if (c == '\r') {
                    continue; // Ignore carriage returns
                }
                // All metrics from 'info' are unscaled. We apply the scale to them.
                // bearing.y (yoff) is the vertical offset FROM THE BASELINE to the top of the glyph.
                const float baselineY = cursorY + (font->getAscent() * scale);
                float xpos = cursorX + info.bearing.x * scale;
                float ypos = baselineY + info.bearing.y * scale; // The top of the quad
                float w = info.size.x * scale;
                float h = info.size.y * scale;

                // Define the four corners of the quad for this character.
                // This assumes a Y-down coordinate system (top-left is 0,0).
                glm::vec2 topLeft = {xpos, ypos};
                glm::vec2 bottomLeft = {xpos, ypos + h};
                glm::vec2 bottomRight = {xpos + w, ypos + h};
                glm::vec2 topRight = {xpos + w, ypos};

                // Create vertices in the same order as your original code (TL, BL, BR, TR)
                UIVertex v0, v1, v2, v3;
                v0.position = topLeft;
                v1.position = bottomLeft;
                v2.position = bottomRight;
                v3.position = topRight;

                v0.texCoords = {info.uv0.x, info.uv0.y}; // Top-Left UV
                v1.texCoords = {info.uv0.x, info.uv1.y}; // Bottom-Left UV
                v2.texCoords = {info.uv1.x, info.uv1.y}; // Bottom-Right UV
                v3.texCoords = {info.uv1.x, info.uv0.y}; // Top-Right UV

                v0.color = v1.color = v2.color = v3.color = color;

                // First triangle (TL, BL, BR)
                m_vertices.push_back(v0);
                m_vertices.push_back(v1);
                m_vertices.push_back(v2);

                // Second triangle (BR, TR, TL)
                m_vertices.push_back(v2);
                m_vertices.push_back(v3);
                m_vertices.push_back(v0);

                m_drawCommands.back().vertexCount += 6;

                // Advance the horizontal cursor position for the next character
                cursorX += info.advance * scale;
            } else if (c == ' ') {
                // Handle space character (no glyph, just advance)
                cursorX += (info.advance) * scale;
            } else if (c == '\t') {
                // Handle tab character (advance by a fixed amount, e.g., 4 spaces)
                float spaceAdvance = 0.0f;
                CharInfo spaceInfo;
                if (font->getCharInfo(' ', spaceInfo)) {
                    spaceAdvance = spaceInfo.advance * scale;
                }
                cursorX += spaceAdvance * 4; // Assuming a tab is 4 spaces
            }
        }
    }

    const float UIRenderer::getDpiScale() {
        return m_dpiScale;
    }
}
