#include "ModelLoader.hpp"

#include "MeshLoader.hpp"
#include "MaterialLoader.hpp"
#include "TextureLoader.hpp"
#include "serialization/BufferStream.hpp"

namespace TechEngine {
    static std::filesystem::path sanitise(const std::string& s) {
        std::string result = s;
        for (char& c: result)
            if (c == '/' || c == '\\' || c == ':' || c == ' ') c = '_';
        return std::filesystem::path(result);
    }

    ModelLoader::ModelLoader(ResourceSystem& resourceSystem,
                             FileSystem& fileSystem,
                             TextureLoader& textureLoader,
                             MaterialLoader& materialLoader,
                             MeshLoader& meshLoader)
        : m_fileSystem(fileSystem),
          m_resourcesSystem(resourceSystem),
          m_assimpLoader(resourceSystem, fileSystem, textureLoader),
          m_textureLoader(textureLoader),
          m_materialLoader(materialLoader),
          m_meshLoader(meshLoader) {
    }

    ModelLoader::~ModelLoader() = default;

    void ModelLoader::importModelFile(const std::filesystem::path& virtualPath) {
        FileStatus fileStatus = m_fileSystem.status(virtualPath);
        const std::string modelName = fileStatus.name.string();

        if (m_resourcesSystem.isModelRegistered(modelName)) {
            return;
        }

        const std::filesystem::path parent = m_fileSystem.getParentPath(virtualPath);
        const std::filesystem::path modelDir = parent / modelName;

        const std::filesystem::path meshDir = modelDir / "meshes";
        const std::filesystem::path materialDir = modelDir / "materials";
        const std::filesystem::path textureDir = modelDir / "textures";

        AssimpLoader::SceneData sceneData = m_assimpLoader.loadSourceFile(virtualPath);

        auto textureMap = importTextures(sceneData, textureDir);
        auto materialMap = importMaterials(sceneData, materialDir);

        ModelNode rootNode = convertNode(sceneData.rootNode, meshDir, materialMap, modelName, "");

        auto modelResource = std::make_shared<ModelResource>(modelName);
        modelResource->setRootNode(std::move(rootNode));
        m_resourcesSystem.registerModelResource(modelResource);

        Buffer modelBuffer;
        serializeModelResource(modelBuffer, modelResource);
        m_fileSystem.write(parent / (modelName + resourceExtension()), modelBuffer);

        TE_LOGGER_INFO("Imported model '{0}': {1} textures, {2} materials",
                       modelName, textureMap.size(), materialMap.size());
    }

    std::unordered_map<std::string, std::shared_ptr<TextureResource>>
    ModelLoader::importTextures(const AssimpLoader::SceneData& sceneData,
                                const std::filesystem::path& outPath) const {
        std::unordered_map<std::string, std::shared_ptr<TextureResource>> result;

        for (const auto& [name, texture]: sceneData.textures) {
            if (!texture) continue;

            m_resourcesSystem.registerTextureResource(texture);

            Buffer buf;
            m_textureLoader.serializeTextureResource(buf, texture);
            m_fileSystem.write(outPath / (name + m_textureLoader.resourceExtension()), buf);

            result[name] = texture;
        }
        return result;
    }

    std::unordered_map<std::string, std::shared_ptr<MaterialResource>>
    ModelLoader::importMaterials(const AssimpLoader::SceneData& sceneData,
                                 const std::filesystem::path& outDir) const {
        std::unordered_map<std::string, std::shared_ptr<MaterialResource>> result;

        for (const auto& [name, material]: sceneData.materials) {
            if (!material) continue;

            m_resourcesSystem.registerMaterialResource(material);

            Buffer buf;
            m_materialLoader.serializeMaterialResource(buf, material);
            m_fileSystem.write(outDir / (name + m_materialLoader.resourceExtension()), buf);

            result[name] = material;
        }
        return result;
    }

    ModelNode ModelLoader::convertNode(
        const AssimpLoader::Node& assimpNode,
        const std::filesystem::path& meshOutDir,
        const std::unordered_map<std::string, std::shared_ptr<MaterialResource>>& materialMap,
        const std::filesystem::path& modelName,
        const std::filesystem::path& nodePathPrefix) {
        ModelNode modelNode;
        modelNode.name = assimpNode.name;
        modelNode.position = assimpNode.position;
        modelNode.rotation = glm::quat(glm::radians(assimpNode.rotation));
        modelNode.scale = assimpNode.scale;
        modelNode.meshUUID = UUID(-1);
        modelNode.sourceMaterialUUID = UUID(-1);
        const std::filesystem::path nodePath = nodePathPrefix.empty()
                                                   ? sanitise(assimpNode.name)
                                                   : std::filesystem::path(nodePathPrefix.string() + "_" + sanitise(assimpNode.name).string());


        for (size_t i = 0; i < assimpNode.meshes.size(); ++i) {
            const AssimpLoader::aiMeshData& srcMesh = assimpNode.meshes[i];

            UUID materialUUID;
            auto matIt = materialMap.find(srcMesh.material);
            if (matIt != materialMap.end()) {
                materialUUID = matIt->second->getUUID();
            } else {
                TE_LOGGER_WARN("  Material '{0}' not found for mesh on node '{1}'", srcMesh.material, assimpNode.name);
            }

            const std::string meshName = srcMesh.name;
            std::shared_ptr<MeshResource> meshResource;
            if (m_resourcesSystem.isMeshRegistered(meshName)) {
                meshResource = m_resourcesSystem.getMeshResource(meshName);
            } else {
                meshResource = std::make_shared<MeshResource>(meshName, srcMesh.vertices, srcMesh.indices);
                m_resourcesSystem.registerMeshResource(meshResource);

                Buffer meshBuf;
                m_meshLoader.serializeMeshResource(meshBuf, meshResource);
                m_fileSystem.write(meshOutDir / (meshName + m_meshLoader.resourceExtension()), meshBuf);
            }


            ModelNode meshNode;
            meshNode.name = meshName;
            meshNode.meshUUID = meshResource->getUUID();
            meshNode.sourceMaterialUUID = materialUUID;

            modelNode.children.emplace_back(std::move(meshNode));
        }

        for (const AssimpLoader::Node& child: assimpNode.children) {
            modelNode.children.emplace_back(convertNode(child, meshOutDir, materialMap, modelName, nodePath));
        }

        return modelNode;
    }

    void ModelLoader::serializeModelResource(Buffer& buffer, const std::shared_ptr<ModelResource>& model) {
        if (model) {
            std::unique_ptr<BufferStreamWriter> writer = std::make_unique<BufferStreamWriter>(buffer, 0);
            ModelResource::serialize(writer.get(), *model);
        }
    }

    std::shared_ptr<ModelResource> ModelLoader::deserializeModelResource(Buffer& buffer) const {
        BufferStreamReader reader(buffer, 0);
        auto model = ModelResource::deserialize(&reader);
        m_resourcesSystem.registerModelResource(model);
        return model;
    }

    std::vector<std::string> ModelLoader::sourceExtensions() const {
        return {".obj", ".fbx", ".gltf", ".glb"};
    }

    std::string ModelLoader::resourceExtension() const {
        return ".temodel";
    }
}
