#include "TextureLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "serialization/BufferStream.hpp"


namespace TechEngine {
    TextureLoader::TextureLoader(ResourceSystem& m_resourcesSystem,
                                 FileSystem& m_fileSystem) : m_resourcesSystem(m_resourcesSystem),
                                                             m_fileSystem(m_fileSystem) {
    }

    void TextureLoader::importTextureFile(const std::filesystem::path& virtualPath) {
        Buffer buffer;
        FileStatus fileStatus = m_fileSystem.status(virtualPath);
        const std::filesystem::path parent = m_fileSystem.getParentPath(virtualPath);

        std::shared_ptr<TextureResource> texture = loadTextureFromImageFile(virtualPath, fileStatus.name.string());
        m_resourcesSystem.registerTextureResource(texture);
        serializeTextureResource(buffer, texture);

        std::filesystem::path texturePath = parent / (fileStatus.name.string() + resourceExtension());
        m_fileSystem.write(texturePath, buffer);
    }

    std::shared_ptr<TextureResource> TextureLoader::loadTextureFromImageFile(const std::filesystem::path& imageVirtualPath, const std::string& name) const {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);

        std::filesystem::path absoluteImagePath = m_fileSystem.resolve(imageVirtualPath);
        std::string extension = absoluteImagePath.extension().string();
        TextureType type;

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
            std::vector<unsigned char> pixelData;
            type = (extension == ".png") ? PNG : JPG;
            stbi_uc* data = stbi_load(absoluteImagePath.string().c_str(), &width, &height, &channels, 0);
            pixelData.assign(data, data + (width * height * channels));

            stbi_image_free(data);
            return std::make_shared<TextureResource>(name, width, height, channels, type, CLAMP_TO_EDGE, 1.0f, 1.0f, pixelData);
        } else if (extension == ".hdr") {
            std::vector<float> pixelDataFloat;
            type = HDR;
            float* data = stbi_loadf(absoluteImagePath.string().c_str(), &width, &height, &channels, 0);
            pixelDataFloat.assign(data, data + (width * height * channels));

            stbi_image_free(data);
            return std::make_shared<TextureResource>(name, width, height, channels, type, CLAMP_TO_EDGE, 1.0f, 1.0f, pixelDataFloat);
        } else {
            TE_LOGGER_ERROR("Unsupported texture format: {0}", extension);
            return nullptr;
        }
    }

    void TextureLoader::serializeTextureResource(Buffer& buffer, const std::string& textureName) {
        std::shared_ptr<TextureResource> texture = m_resourcesSystem.getTextureResource(textureName);
        serializeTextureResource(buffer, texture);
    }

    void TextureLoader::serializeTextureResource(Buffer& buffer, const std::shared_ptr<TextureResource>& texture) {
        if (texture) {
            BufferStreamWriter streamWrite = BufferStreamWriter(buffer, 0);
            TextureResource::serialize(&streamWrite, *texture.get());
        } else {
            TE_LOGGER_WARN("Texture not found: {0}", texture->getName());
        }
    }

    std::shared_ptr<TextureResource> TextureLoader::deserializeTextureResource(Buffer& buffer) const {
        BufferStreamReader streamRead = BufferStreamReader(buffer, 0);
        std::shared_ptr<TextureResource> texture = std::make_shared<TextureResource>("", 0, 0, 0, TextureType::PNG, CLAMP_TO_EDGE, 1.0f, 1.0f, std::vector<float>());
        TextureResource::deserialize(&streamRead, *texture);
        m_resourcesSystem.registerTextureResource(texture);
        return texture;
    }

    std::vector<std::string> TextureLoader::sourceExtensions() const {
        return {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".hdr"};
    }

    std::string TextureLoader::resourceExtension() const {
        return ".tetexture";
    }
}
