#pragma once
#include <cstdint>


namespace TechEngine {
    class TextureResource;

    class Texture {
    private:
        uint32_t m_id;
        uint64_t m_handle;
        bool m_isResident;

    public:
        Texture();

        void uploadFromResource(const TextureResource& textureResource);

        void makeResident();

        void makeNonResident();

        uint64_t getHandle() const;

        uint32_t getID() const;

        bool isResident() const;
    };
}
