#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <unordered_set>
#include <glm/glm.hpp>

namespace TechEngine {
    class UIRenderInterface : public Rml::RenderInterface {
    private:
        struct CompiledGeometryData {
            unsigned int VAO = 0; // Vertex Array Object
            unsigned int VBO = 0; // Vertex Buffer Object
            unsigned int EBO = 0; // Element Buffer Object
            int NumIndices = 0;
        };

        unsigned int m_shaderProgram = 0;
        int m_transformUniformLoc = -1;
        int m_translationUniformLoc = -1;

        std::vector<CompiledGeometryData> m_compiledGeometries;
        std::unordered_set<unsigned int> m_managedTextures;
        uint32_t width = 1280;
        uint32_t height = 720;
        glm::mat4 m_projectionMatrix;
        unsigned int m_whiteTexture = 0;

    public:
        UIRenderInterface();

        ~UIRenderInterface() override;

        void Initialize();

        void Shutdown();

        Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;

        void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;

        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

        Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;

        Rml::TextureHandle GenerateTexture(Rml::Span<const unsigned char> source, Rml::Vector2i source_dimensions) override;

        void ReleaseTexture(Rml::TextureHandle texture) override;

        void EnableScissorRegion(bool enable) override;

        void SetScissorRegion(Rml::Rectanglei region) override;

        void SetViewport(int width, int height);
    };
}
