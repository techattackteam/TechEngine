#pragma once

#include "AssimpLoader.hpp"
#include "resources/IResourceLoader.hpp"
#include "resources/ResourceSystem.hpp"
#include "TechEngine/core/resources/model/ModelResource.hpp"

namespace TechEngine {
    class ModelLoader : public IResourceLoader {
    public:
        ModelLoader(ResourceSystem& resourceSystem,
                    FileSystem& fileSystem,
                    TextureLoader& textureLoader,
                    MaterialLoader& materialLoader,
                    MeshLoader& meshLoader);

        ~ModelLoader() override;

        void importModelFile(const std::filesystem::path& virtualPath);

        void serializeModelResource(Buffer& buffer, const std::shared_ptr<ModelResource>& model);

        std::shared_ptr<ModelResource> deserializeModelResource(Buffer& buffer) const;

        std::vector<std::string> sourceExtensions() const override; // Same has Mesh Loader

        std::string resourceExtension() const override;

    private:
        FileSystem& m_fileSystem;
        ResourceSystem& m_resourcesSystem;
        AssimpLoader m_assimpLoader;
        TextureLoader& m_textureLoader;
        MaterialLoader& m_materialLoader;
        MeshLoader& m_meshLoader;

        std::unordered_map<std::string, std::shared_ptr<TextureResource>>
        importTextures(const AssimpLoader::SceneData& sceneData,
                       const std::filesystem::path& outPath) const;

        std::unordered_map<std::string, std::shared_ptr<MaterialResource>>
        importMaterials(const AssimpLoader::SceneData& sceneData,
                        const std::filesystem::path& outDir) const;

        ModelNode convertNode(
            const AssimpLoader::Node& assimpNode,
            const std::filesystem::path& meshOutDir,
            const std::unordered_map<std::string, std::shared_ptr<MaterialResource>>& materialMap,
            const std::filesystem::path& modelName,
            const std::filesystem::path& nodePathPrefix);
    };
}
