#pragma once
#include <cstdint>
#include <GL/glew.h>

#include "IGPUResource.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"

namespace TechEngine {
    class Texture : public IGPUResource {
    private:
        uint32_t m_id;
        uint64_t m_handle;
        bool m_isResident;
        GLenum m_target;

        uint32_t m_width;
        uint32_t m_height;

    public:
        Texture();

        Texture(const UUID& resourceUUID);

        void uploadFromResource(const TextureResource& textureResource);

        void create(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLenum wrapU, GLenum wrapV, const void* data = nullptr, GLsizei depth = 0);

        void generateMipMap();

        void makeResident();

        void makeNonResident();

        void deleteTexture();

        uint64_t getHandle() const;

        uint32_t getID() const;

        uint32_t getID();

        GLenum getTarget() const;

        uint32_t getWidth() const;

        uint32_t getHeight() const;

        bool isResident() const;

    private:
        GLenum getOpenGlWrapMode(const TextureMode& mode);
    };
}
