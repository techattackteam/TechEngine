#include "TechEngine/core/resources/texture/TextureResource.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    TextureResource::TextureResource(std::string name,
                                     uint32_t id,
                                     int width,
                                     int height,
                                     int channels,
                                     std::vector<unsigned char> pixelData) : m_name(name),
                                                                             m_id(id),
                                                                             m_width(width),
                                                                             m_height(height),
                                                                             m_channels(channels),
                                                                             m_pixelData(std::move(pixelData)) {
    }

    void TextureResource::freePixels() {
        m_pixelData.clear();
        m_pixelData.shrink_to_fit();
    }

    uint32_t TextureResource::getID() const {
        return m_id;
    }

    std::string TextureResource::getName() const {
        return m_name;
    }

    const std::vector<unsigned char> TextureResource::getPixels() const {
        return m_pixelData;
    }

    bool TextureResource::hasPixelData() const {
        return !m_pixelData.empty();
    }

    int TextureResource::getWidth() const {
        return m_width;
    }

    int TextureResource::getHeight() const {
        return m_height;
    }

    int TextureResource::getChannels() const {
        return m_channels;
    }
}
