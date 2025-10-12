#pragma once

#include <memory>
#include <unordered_map>
#include <glm/vec2.hpp>

#include "TechEngine/client/core/ExportDLL.hpp"

#include "GLFW.hpp"

namespace TechEngine {
    class Texture;

    struct CLIENT_DLL AttachmentInfo {
        bool isRenderBuffer;
        GLint internalFormat;

        GLenum format;
        GLenum type;
        GLenum target;
        bool multisample = false;
    };

    class CLIENT_DLL FrameBuffer {
    private:
        uint32_t m_id = 0;
        uint32_t m_width = 0;
        uint32_t m_height = 0;

        std::unordered_map<GLenum, std::unique_ptr<Texture>> m_textures;
        std::unordered_map<GLenum, const Texture*> m_externalTextures;
        std::unordered_map<GLenum, uint32_t> m_renderBuffers;
        std::unordered_map<GLenum, AttachmentInfo> m_attachments;

    public:
        FrameBuffer() = default;

        FrameBuffer(const FrameBuffer& other) ;

        FrameBuffer& operator=(const FrameBuffer& other);

        ~FrameBuffer();

        void init(uint32_t width, uint32_t height);

        void release();

        void bind();

        void bindDraw() const;

        void bindRead();

        void unBind();

        void resize(uint32_t width, uint32_t height);

        void clear(GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        uint32_t attachTexture(GLenum attachmentPoint, GLenum target, GLint internalFormat, GLenum format, GLenum type, bool multisample, int samples);

        void attachExternalTexture(GLenum attachmentPoint, const Texture& texture, int layer = 0, int mipLevel = 0);

        void attachTextureLayer(GLenum attachmentPoint, const Texture& texture, int layer, int mipLevel = 0);

        uint32_t attachRenderBuffer(GLenum attachmentPoint, GLint internalFormat, uint32_t width, uint32_t height, int samples);

        bool validate() const;

        uint32_t getID();

        uint32_t getWidth() const;

        uint32_t getHeight() const;

        glm::uvec2 getSize() const;

        Texture* getTexture(GLenum attachmentPoint);

        const Texture* getTexture(GLenum attachmentPoint) const;

        uint32_t getTextureID(GLenum attachmentPoint) const;

        bool hasTexture(GLenum attachmentPoint) const;

        bool hasAttachment(GLenum attachmentPoint) const;

    private:
        void updateDrawBuffers();
    };
}
