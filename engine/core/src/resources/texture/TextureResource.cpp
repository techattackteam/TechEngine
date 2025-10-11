#include "TechEngine/core/resources/texture/TextureResource.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    TextureResource::TextureResource(std::string name,
                                     uint32_t id,
                                     int width,
                                     int height,
                                     int channels,
                                     TextureType type,
                                     std::vector<unsigned char> pixelData) : m_name(name),
                                                                             m_id(id),
                                                                             m_width(width),
                                                                             m_height(height),
                                                                             m_channels(channels),
                                                                             m_type(type),
                                                                             m_pixelData(std::move(pixelData)) {
    }

    TextureResource::TextureResource(std::string name,
                                     uint32_t id,
                                     int width,
                                     int height,
                                     int channels,
                                     TextureType type,
                                     std::vector<float> pixelData) : m_name(name),
                                                                     m_id(id),
                                                                     m_width(width),
                                                                     m_height(height),
                                                                     m_channels(channels),
                                                                     m_type(type),
                                                                     m_pixelDataFloat(std::move(pixelData)) {
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

    const std::vector<unsigned char> TextureResource::getPixelsChar() const {
        return m_pixelData;
    }

    const std::vector<float> TextureResource::getPixelsFloat() const {
        return m_pixelDataFloat;
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

    TextureType TextureResource::getType() const {
        return m_type;
    }
}
