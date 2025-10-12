#pragma once
#include <cstdint>
#include <GL/glew.h>


namespace TechEngine {
    class TextureResource;

    class Texture {
    private:
        uint32_t m_id;
        uint64_t m_handle;
        bool m_isResident;
        GLenum m_target;

    public:
        Texture();

        void uploadFromResource(const TextureResource& textureResource);

        void create(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data = nullptr);

        void generateMipMap();

        void makeResident();

        void makeNonResident();

        void deleteTexture();

        uint64_t getHandle() const;

        uint32_t getID() const;

        uint32_t getID();

        GLenum getTarget() const;

        bool isResident() const;
    };
}
