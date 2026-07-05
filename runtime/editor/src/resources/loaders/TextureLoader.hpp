#pragma once

#include "resources/IResourceLoader.hpp"
#include "resources/ResourceSystem.hpp"
#include "fileSystem/FileSystem.hpp"

namespace TechEngine {
    class TextureLoader : public IResourceLoader {
    private:
        ResourceSystem& m_resourcesSystem;
        FileSystem& m_fileSystem;

    public:
        TextureLoader(ResourceSystem& m_resourcesSystem, FileSystem& m_fileSystem);

        ~TextureLoader() override = default;

        void importTextureFile(const std::filesystem::path& virtualPath);

        std::shared_ptr<TextureResource> loadTextureFromImageFile(const std::filesystem::path& imageVirtualPath, const std::string& name) const;

        void serializeTextureResource(Buffer& buffer, const std::string& textureName);

        void serializeTextureResource(Buffer& buffer, const std::shared_ptr<TextureResource>& texture);

        std::shared_ptr<TextureResource> deserializeTextureResource(Buffer& buffer) const;

        std::vector<std::string> sourceExtensions() const override;

        std::string resourceExtension() const override;
    };
}
