#pragma once


#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"
#include "TechEngine/core/resources/IResource.hpp"

namespace TechEngine {
    enum TextureType {
        PNG,
        JPG,
        HDR
    };

    enum TextureMode {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER
    };

    class CORE_DLL TextureResource : public IResource {
    private:
        friend class TextureLoader;

        std::vector<unsigned char> m_pixelData;
        std::vector<float> m_pixelDataFloat;

        int m_width;
        int m_height;
        int m_channels;
        TextureType m_type;
        TextureMode m_modeU;
        TextureMode m_modeV;
        float m_wrapS = 1.0f;
        float m_wrapT = 1.0f;

    public:
        TextureResource(const std::string& name, int width, int height, int channels, TextureType type, TextureMode mode, float wrapS, float wrapT, std::vector<unsigned char> pixelData);

        TextureResource(const std::string& name, int width, int height, int channels, TextureType type, TextureMode mode, float wrapS, float wrapT, std::vector<float> pixelData);

        void freePixels();

        void setMapMode(TextureMode modeU, TextureMode modeV);

        const std::vector<unsigned char> getPixelsChar() const;

        const std::vector<float> getPixelsFloat() const;

        bool hasPixelData() const;

        int getWidth() const;

        int getHeight() const;

        int getChannels() const;

        TextureMode getModeU() const;

        TextureMode getModeV() const;

        TextureType getType() const;

    private:
        static void serialize(StreamWriter* writer, const TextureResource& texture);

        static void deserialize(StreamReader* reader, TextureResource& texture);
    };
}
