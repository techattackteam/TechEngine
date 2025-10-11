#pragma once

#include <string>

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include <vector>


namespace TechEngine {
    enum TextureType {
        PNG,
        JPG,
        HDR
    };

    class CORE_DLL TextureResource {
    private:
        uint32_t m_id;

        std::vector<unsigned char> m_pixelData;
        std::vector<float> m_pixelDataFloat;

        std::string m_name;
        int m_width;
        int m_height;
        int m_channels;
        TextureType m_type;

    public:
        TextureResource(std::string name, uint32_t id, int width, int height, int channels, TextureType type, std::vector<unsigned char> pixelData);

        TextureResource(std::string name, uint32_t id, int width, int height, int channels, TextureType type, std::vector<float> pixelData);

        void freePixels();

        uint32_t getID() const;

        std::string getName() const;

        const std::vector<unsigned char> getPixelsChar() const;

        const std::vector<float> getPixelsFloat() const;

        bool hasPixelData() const;

        int getWidth() const;

        int getHeight() const;

        int getChannels() const;

        TextureType getType() const;
    };
}
