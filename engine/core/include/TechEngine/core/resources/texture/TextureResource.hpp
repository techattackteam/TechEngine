#pragma once

#include <string>

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include <vector>


namespace TechEngine {
    class CORE_DLL TextureResource {
    private:
        uint32_t m_id;

        std::vector<unsigned char> m_pixelData;

        std::string m_name;
        int m_width;
        int m_height;
        int m_channels;

    public:
        TextureResource(std::string name, uint32_t id, int width, int height, int channels, std::vector<unsigned char> pixelData);

        void freePixels();

        uint32_t getID() const;

        std::string getName() const;

        const std::vector<unsigned char> getPixels() const;

        bool hasPixelData() const;

        int getWidth() const;

        int getHeight() const;

        int getChannels() const;
    };
}
