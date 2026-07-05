#pragma once
#include "fileSystem/FileSystem.hpp"
#include "resources/IResourceLoader.hpp"
#include "resources/ResourceSystem.hpp"

namespace TechEngine {
    class MaterialLoader : public IResourceLoader {
    private:
        ResourceSystem& m_resourcesSystem;
        FileSystem& m_fileSystem;

    public:
        MaterialLoader(ResourceSystem& m_resourcesSystem, FileSystem& m_fileSystem);

        ~MaterialLoader() override = default;

        void createNewMaterialResource(const std::filesystem::path& virtualPath);

        void serializeMaterialResource(Buffer& buffer, const std::string& materialName);

        void serializeMaterialResource(Buffer& buffer, const std::shared_ptr<MaterialResource>& material);

        std::shared_ptr<MaterialResource> deserializeMaterialResource(Buffer& buffer) const;

        std::vector<std::string> sourceExtensions() const override;

        std::string resourceExtension() const override;
    };
}
