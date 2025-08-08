#include "UIRenderInterface.hpp"

#include "GL/glew.h"
#include "renderer/ErrorCatcher.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "window/Window.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace TechEngine {
    UIRenderInterface::UIRenderInterface() {
    }

    UIRenderInterface::~UIRenderInterface() {
    }

    void UIRenderInterface::Initialize() {
        const char* vsSource = R"glsl(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            layout (location = 2) in vec4 aColor;

            out vec2 vTexCoord;
            out vec4 vColor;

            uniform mat4 u_transform;
            uniform vec2 u_translation;

            void main() {
                vTexCoord = aTexCoord;
                vColor = aColor;
                vec2 pos = aPos + u_translation;
                gl_Position = u_transform * vec4(pos, 0.0, 1.0);
            }
        )glsl";

        const char* fsSource = R"glsl(
            #version 330 core
            out vec4 FragColor;

            in vec2 vTexCoord;
            in vec4 vColor;

            uniform sampler2D u_texture;

            void main() {
                vec4 texColor = texture(u_texture, vTexCoord);
                FragColor = vColor * texColor;
            }
        )glsl";

        // Create and compile shaders...
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vsSource, NULL);
        glCompileShader(vertexShader);
        // ... error checking ...

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fsSource, NULL);
        glCompileShader(fragmentShader);
        // ... error checking ...

        // Link shaders into a program
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);
        glLinkProgram(m_shaderProgram);
        // ... error checking ...

        // Detach and delete shaders as they are now linked
        glDetachShader(m_shaderProgram, vertexShader);
        glDetachShader(m_shaderProgram, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // Get uniform locations
        m_transformUniformLoc = glGetUniformLocation(m_shaderProgram, "u_transform");
        m_translationUniformLoc = glGetUniformLocation(m_shaderProgram, "u_translation");
        glGenTextures(1, &m_whiteTexture);
        glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
        unsigned char whitePixel[] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_projectionMatrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    }

    void UIRenderInterface::Shutdown() {
        if (m_shaderProgram == 0) return; // Already shut down

        // Delete the shader program
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;

        // Clean up any remaining compiled geometry
        for (const auto& geom: m_compiledGeometries) {
            if (geom.VAO != 0) { // Check if it's a valid entry
                glDeleteVertexArrays(1, &geom.VAO);
                glDeleteBuffers(1, &geom.VBO);
                glDeleteBuffers(1, &geom.EBO);
            }
        }
        m_compiledGeometries.clear();

        // Clean up any remaining textures
        for (unsigned int textureId: m_managedTextures) {
            glDeleteTextures(1, &textureId);
        }
        m_managedTextures.clear();
        glDeleteTextures(1, &m_whiteTexture);
        m_whiteTexture = 0;
    }

    Rml::CompiledGeometryHandle UIRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
        CompiledGeometryData data;
        data.NumIndices = static_cast<int>(indices.size());

        GlCall(glGenVertexArrays(1, &data.VAO));
        GlCall(glGenBuffers(1,&data.VBO));
        GlCall(glGenBuffers(1, &data.EBO));

        GlCall(glBindVertexArray(data.VAO));

        GlCall(glBindBuffer(GL_ARRAY_BUFFER, data.VBO));
        GlCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Rml::Vertex), vertices.data(), GL_STATIC_DRAW));

        GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.EBO));
        GlCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW));

        GlCall(glEnableVertexAttribArray(0));
        GlCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, position)));
        GlCall(glEnableVertexAttribArray(1));
        GlCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, tex_coord)));
        GlCall(glEnableVertexAttribArray(2));
        GlCall(glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex), (void*)offsetof(Rml::Vertex, colour)));

        GlCall(glBindVertexArray(0));

        m_compiledGeometries.push_back(data);
        return (Rml::CompiledGeometryHandle)(m_compiledGeometries.size());
    }

    void UIRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometryHandle, Rml::Vector2f translation, Rml::TextureHandle texture) {
        size_t geometry_index = (size_t)geometryHandle - 1;
        if (geometry_index >= m_compiledGeometries.size()) {
            return; // Invalid handle
        }
        const CompiledGeometryData& geometry = m_compiledGeometries[geometry_index];

        GlCall(glUseProgram(m_shaderProgram));
        GlCall(glEnable(GL_BLEND));
        GlCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GlCall(glDisable(GL_DEPTH_TEST));

        glUniformMatrix4fv(m_transformUniformLoc, 1, GL_FALSE, &m_projectionMatrix[0][0]);
        glUniform2f(m_translationUniformLoc, translation.x, translation.y);

        unsigned int textureId = (unsigned int)(uintptr_t)texture;
        if (textureId == 0) {
            textureId = m_whiteTexture;
        }
        GlCall(glBindTexture(GL_TEXTURE_2D, textureId));

        GlCall(glBindVertexArray(geometry.VAO));
        GlCall(glDrawElements(GL_TRIANGLES, geometry.NumIndices, GL_UNSIGNED_INT, nullptr));

        // --- Restore render state ---
        GlCall(glBindVertexArray(0));
        GlCall(glEnable(GL_DEPTH_TEST));
        GlCall(glDisable(GL_BLEND));
    }

    void UIRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
        size_t geometry_index = (size_t)geometry - 1;
        if (geometry_index < m_compiledGeometries.size()) {
            const auto& geom = m_compiledGeometries[geometry_index];
            GlCall(glDeleteVertexArrays(1, &geom.VAO));
            GlCall(glDeleteBuffers(1, &geom.VBO));
            GlCall(glDeleteBuffers(1, &geom.EBO));
            // We can't just erase from the vector as it would invalidate other handles.
            // A more robust implementation might use a map or a free-list,
            // but for now, just marking it as empty is fine.
            m_compiledGeometries[geometry_index] = {};
        }
    }

    Rml::TextureHandle UIRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) {
        // RmlUi gives file paths relative to the executable or the document.
        // For stb_image, we need a C-style string.
        const char* file_path = source.c_str();

        int width, height, channels;
        // Use stbi_load to load the image file from disk.
        // stbi_set_flip_vertically_on_load(true) might be needed depending on your coordinate system.
        unsigned char* image_data = stbi_load(file_path, &width, &height, &channels, 0);

        if (!image_data) {
            // You should have a logging system here!
            // Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to load image: %s", file_path);
            stbi_image_free(image_data);
            return 0; // Return a null handle
        }

        texture_dimensions = {width, height};

        // Determine the OpenGL format based on the number of channels
        GLenum format = GL_RGBA;
        if (channels == 3) {
            format = GL_RGB;
        } else if (channels == 1) {
            format = GL_RED;
        }

        // Generate the texture from the loaded data.
        // We can reuse our GenerateTexture function!
        Rml::Span<const Rml::byte> data_span(image_data, width * height * channels);
        Rml::TextureHandle handle = GenerateTexture(data_span, texture_dimensions);

        // After the data is uploaded to the GPU, we can free the memory from the CPU.
        stbi_image_free(image_data);

        return handle;
    }

    Rml::TextureHandle UIRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source_data, Rml::Vector2i source_dimensions) {
        GLuint texture_id = 0;
        glGenTextures(1, &texture_id);
        if (texture_id == 0) {
            // Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, texture_id);

        // Determine the format. Font atlases generated by RmlUi are single-channel (grayscale).
        // The source data from LoadTexture could be RGB or RGBA. A simple check helps.
        GLenum format = GL_RGBA;
        if (source_data.size() == (size_t)(source_dimensions.x * source_dimensions.y)) {
            format = GL_RED;
            // Tell OpenGL how to unpack single-channel data
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        } else if (source_data.size() == (size_t)(source_dimensions.x * source_dimensions.y * 3)) {
            format = GL_RGB;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        } else {
            // Assume RGBA
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        }

        // Upload the pixel data to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, format, source_dimensions.x, source_dimensions.y, 0, format, GL_UNSIGNED_BYTE, source_data.data());

        // Set texture parameters. These are good defaults for UI.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Reset pixel store alignment to default
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glBindTexture(GL_TEXTURE_2D, 0);

        // Keep track of the texture ID so we can release it later
        m_managedTextures.insert(texture_id);

        // RmlUi just wants a number. The raw OpenGL ID is perfect.
        return (Rml::TextureHandle)texture_id;
    }

    void UIRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle) {
        // Cast the handle back to our OpenGL ID
        GLuint texture_id = (GLuint)(uintptr_t)texture_handle;

        // Check if it's one we are managing
        auto it = m_managedTextures.find(texture_id);
        if (it != m_managedTextures.end()) {
            glDeleteTextures(1, &texture_id);
            m_managedTextures.erase(it);
        }
    }

    void UIRenderInterface::EnableScissorRegion(bool enable) {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    void UIRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
        int windowHeight = height;
        glScissor(region.p0.x, windowHeight - (region.p0.y + region.Height()), region.Width(), region.Height());
    }

    void UIRenderInterface::SetViewport(int width, int height) {
        glViewport(0, 0, width, height);
        this->width = width;
        this->height = height;
        m_projectionMatrix = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    }
}
