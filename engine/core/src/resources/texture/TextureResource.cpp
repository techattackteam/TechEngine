#include "TechEngine/core/resources/texture/TextureResource.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    TextureResource::TextureResource(const std::string& name,
                                     const int width,
                                     const int height,
                                     const int channels,
                                     const TextureType type,
                                     const TextureMode mode,
                                     const float wrapS,
                                     const float wrapT,
                                     std::vector<unsigned char> pixelData) : m_width(width),
                                                                             m_height(height),
                                                                             m_channels(channels),
                                                                             m_type(type),
                                                                             m_modeU(mode),
                                                                             m_modeV(mode),
                                                                             m_wrapS(wrapS),
                                                                             m_wrapT(wrapT),
                                                                             m_pixelData(std::move(pixelData)),
                                                                             IResource(name) {
    }

    TextureResource::TextureResource(const std::string& name,
                                     const int width,
                                     const int height,
                                     const int channels,
                                     const TextureType type,
                                     const TextureMode mode,
                                     const float wrapS,
                                     const float wrapT,
                                     std::vector<float> pixelData) : m_width(width),
                                                                     m_height(height),
                                                                     m_channels(channels),
                                                                     m_type(type),
                                                                     m_modeU(mode),
                                                                     m_modeV(mode),
                                                                     m_wrapS(wrapS),
                                                                     m_wrapT(wrapT),
                                                                     m_pixelDataFloat(std::move(pixelData)),
                                                                     IResource(name) {
    }

    void TextureResource::freePixels() {
        m_pixelData.clear();
        m_pixelData.shrink_to_fit();
    }

    void TextureResource::setMapMode(TextureMode modeU, TextureMode modeV) {
        m_modeU = modeU;
        m_modeV = modeV;
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

    TextureMode TextureResource::getModeU() const {
        return m_modeU;
    }

    TextureMode TextureResource::getModeV() const {
        return m_modeV;
    }

    TextureType TextureResource::getType() const {
        return m_type;
    }

    void TextureResource::serialize(StreamWriter* writer, const TextureResource& texture) {
        IResource::serialize(writer, texture);
        writer->writeRaw(texture.m_width);
        writer->writeRaw(texture.m_height);
        writer->writeRaw(texture.m_channels);
        writer->writeRaw(static_cast<int>(texture.m_type));
        writer->writeRaw(static_cast<int>(texture.m_modeU));
        writer->writeRaw(static_cast<int>(texture.m_modeV));
        writer->writeRaw(texture.m_wrapS);
        writer->writeRaw(texture.m_wrapT);
        if (texture.m_type == TextureType::HDR) {
            writer->writeArray(texture.m_pixelDataFloat);
        } else {
            writer->writeArray(texture.m_pixelData);
        }
    }

    void TextureResource::deserialize(StreamReader* reader, TextureResource& texture) {
        IResource::deserialize(reader, texture);
        reader->readRaw(texture.m_width);
        reader->readRaw(texture.m_height);
        reader->readRaw(texture.m_channels);
        reader->readRaw(texture.m_type);
        reader->readRaw(texture.m_modeU);
        reader->readRaw(texture.m_modeV);
        reader->readRaw(texture.m_wrapS);
        reader->readRaw(texture.m_wrapT);
        if (texture.m_type == HDR) {
            reader->readArray(texture.m_pixelDataFloat);
        } else {
            reader->readArray(texture.m_pixelData);
        }
    }
}
