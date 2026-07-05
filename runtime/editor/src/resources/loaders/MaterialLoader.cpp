#include "MaterialLoader.hpp"

#include "serialization/BufferStream.hpp"

namespace TechEngine {
    MaterialLoader::MaterialLoader(ResourceSystem& resourcesSystem,
                                   FileSystem& fileSystem) : m_resourcesSystem(resourcesSystem),
                                                             m_fileSystem(fileSystem) {
    }

    void MaterialLoader::createNewMaterialResource(const std::filesystem::path& virtualPath) {
        std::string newMaterialName = m_fileSystem.status(virtualPath).name.string();
        std::shared_ptr<MaterialResource> material = m_resourcesSystem.createMaterialResource(newMaterialName);

        const std::filesystem::path parent = m_fileSystem.getParentPath(virtualPath);
        Buffer buffer;
        serializeMaterialResource(buffer, material);

        std::filesystem::path materialPath = parent / (newMaterialName + resourceExtension());
        m_fileSystem.write(materialPath, buffer);
    }

    void MaterialLoader::serializeMaterialResource(Buffer& buffer, const std::string& materialName) {
        std::shared_ptr<MaterialResource> material = m_resourcesSystem.getMaterialResource(materialName);
        serializeMaterialResource(buffer, material);
    }

    void MaterialLoader::serializeMaterialResource(Buffer& buffer, const std::shared_ptr<MaterialResource>& material) {
        if (material) {
            BufferStreamWriter* streamWrite = new BufferStreamWriter(buffer, 0);
            MaterialResource::serialize(streamWrite, *material.get());
        }
    }

    std::shared_ptr<MaterialResource> MaterialLoader::deserializeMaterialResource(Buffer& buffer) const {
        BufferStreamReader streamRead = BufferStreamReader(buffer, 0);
        std::shared_ptr<MaterialResource> material = MaterialResource::deserialize(&streamRead);
        m_resourcesSystem.registerMaterialResource(material);
        return material;
    }

    std::vector<std::string> MaterialLoader::sourceExtensions() const {
        return {};
    }

    std::string MaterialLoader::resourceExtension() const {
        return ".temat";
    }
}
