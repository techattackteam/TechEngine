#include "ResourcesManager.hpp"


#include "files/FileUtils.hpp"
#include "project/Project.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ResourcesManager::ResourcesManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry),
                                                                           m_meshManager(systemsRegistry),
                                                                           m_materialManager(systemsRegistry),
                                                                           m_textureManager(systemsRegistry) {
    }

    void ResourcesManager::init(AppType appType) {
        std::unordered_map<std::string, std::vector<std::filesystem::path>> filesByExtension = getFilesByExtension(appType);

        m_textureManager.init(filesByExtension[".png"]);
        m_materialManager.init(filesByExtension[".mat"]);
        m_meshManager.init(filesByExtension[".tesmesh"]);
        //assignTextureToMaterial(MaterialManager::DEFAULT_MATERIAL_NAME, "black-streaked-rock1-albedo", "albedo");
        //assignTextureToMaterial(MaterialManager::DEFAULT_MATERIAL_NAME, "black-streaked-rock1-Normal-ogl", "normal");
        //assignTextureToMaterial(MaterialManager::DEFAULT_MATERIAL_NAME, "black-streaked-rock1-Metallic", "metallic");
        //assignTextureToMaterial(MaterialManager::DEFAULT_MATERIAL_NAME, "black-streaked-rock1-Roughness", "roughness");
        //assignTextureToMaterial(MaterialManager::DEFAULT_MATERIAL_NAME, "black-streaked-rock1-ao", "ambient occlusion");
    }

    void ResourcesManager::shutdown() {
        m_materialManager.shutdown();
        m_meshManager.shutdown();
    }

#pragma region MeshManager
    void ResourcesManager::createMesh(const std::string& name) {
        m_meshManager.createMesh(name);
    }

    void ResourcesManager::loadModelFile(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        std::string parentFolder = std::filesystem::path(path).parent_path().string();
        std::string staticMeshPath = parentFolder + "\\" + modelName + ".tesmesh";
        AssimpLoader::ModelData modelData = m_assimpLoader.loadModel(path);
        m_assimpLoader.createStaticMeshFile(modelData.rootNode, staticMeshPath);
        loadStaticMesh(staticMeshPath);
    }

    void ResourcesManager::loadStaticMesh(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        if (m_meshManager.isMeshLoaded(modelName)) {
            TE_LOGGER_WARN("Mesh already registered: {0}", path);
            return;
        }
        m_meshManager.loadStaticMesh(path);
    }

    void ResourcesManager::setVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot set vertices for default mesh");
            return;
        }
        Mesh& mesh = m_meshManager.getMesh(name);
        mesh.setVertices(vertices, indices);
    }

    void ResourcesManager::addVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot add vertices for default mesh");
            return;
        }
        Mesh& mesh = m_meshManager.getMesh(name);
        mesh.addVertices(vertices, indices);
    }

    void ResourcesManager::deleteMesh(const std::string& name) {
        if (name == MeshManager::DEFAULT_MESH_NAME) {
            TE_LOGGER_WARN("Cannot delete default mesh");
            return;
        }
        m_meshManager.deleteMesh(name);
    }

    Mesh& ResourcesManager::getMesh(const std::string& name) {
        if (!m_meshManager.isMeshLoaded(name)) {
            TE_LOGGER_WARN("Mesh not found: {0}", name);
            return getDefaultMesh();
        }
        return m_meshManager.getMesh(name);
    }

    Mesh& ResourcesManager::getDefaultMesh() {
        return m_meshManager.getMesh(MeshManager::DEFAULT_MESH_NAME);
    }

    const std::vector<Mesh>& ResourcesManager::getAllMeshes() {
        return m_meshManager.getMeshes();
    }
#pragma endregion

#pragma region MaterialManager

    Material& ResourcesManager::createMaterial(const std::string& name) {
        return m_materialManager.createMaterial(name);
    }

    void ResourcesManager::loadMaterial(const std::string& name, const std::string& path) {
        if (m_materialManager.materialExists(name)) {
            TE_LOGGER_WARN("Material already registered: {0}", name);
            return;
        }
        m_materialManager.createMaterialFile(name, path);
    }

    Material& ResourcesManager::getMaterial(const std::string& name) {
        if (!m_materialManager.materialExists(name)) {
            TE_LOGGER_WARN("Material not found: {0}", name);
            return getDefaultMaterial();
        }
        return m_materialManager.getMaterial(name);
    }

    Material& ResourcesManager::getDefaultMaterial() {
        return m_materialManager.getMaterial(MaterialManager::DEFAULT_MATERIAL_NAME);
    }

    void ResourcesManager::assignTextureToMaterial(const std::string& materialName, const std::string& textureName, const std::string& textureType) {
        if (!m_materialManager.materialExists(materialName)) {
            TE_LOGGER_WARN("Material not found: {0}", materialName);
            return;
        }
        if (!m_textureManager.textureExists(textureName)) {
            TE_LOGGER_WARN("Texture not found: {0}", textureName);
            return;
        }
        Material& material = m_materialManager.getMaterial(materialName);
        TextureResource& texture = m_textureManager.getTexture(textureName);
        if (textureType == "albedo") {
            material.getAlbedoMapID() = texture.getID();
        } else if (textureType == "normal") {
            material.getNormalMapID() = texture.getID();
        } else if (textureType == "metallic") {
            material.getMetallicMapID() = texture.getID();
        } else if (textureType == "roughness") {
            material.getRoughnessMapID() = texture.getID();
        } else if (textureType == "ambient occlusion") {
            material.getAmbientOcclusionMapID() = texture.getID();
        } else {
            TE_LOGGER_WARN("Unknown texture type: {0}", textureType);
        }
    }

    std::vector<Material*> ResourcesManager::getAllMaterials() {
        return m_materialManager.getMaterials();
    }

#pragma endregion

#pragma region TextureManager
    void ResourcesManager::loadTexture(const std::string& name, const std::string& path) {
        if (m_textureManager.textureExists(name)) {
            TE_LOGGER_WARN("Texture already registered: {0}", name);
            return;
        }
        m_textureManager.loadFromFile(name, path);
    }

    TextureResource& ResourcesManager::getTexture(const std::string& name) {
        if (!m_textureManager.textureExists(name)) {
            TE_LOGGER_WARN("Texture not found: {0}", name);
            throw std::runtime_error("Texture not found");
        }
        return m_textureManager.getTexture(name);
    }

    TextureResource& ResourcesManager::getTexture(const int id) {
        return m_textureManager.getTexture(id);
    }


#pragma endregion

    std::unordered_map<std::string, std::vector<std::filesystem::path>> ResourcesManager::getFilesByExtension(const AppType& appType) {
        std::unordered_map<std::string, std::vector<std::filesystem::path>> filesByExtension;
        std::vector<std::filesystem::path> directories = {
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Assets, appType).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, appType).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Cache, AppType::Common).string(),
            m_systemsRegistry.getSystem<Project>().getPath(PathType::Cache, appType).string(),
        };

        for (const auto& directory: directories) {
            for (const auto& entry: std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    if (filesByExtension.find(extension) == filesByExtension.end()) {
                        filesByExtension[extension] = std::vector<std::filesystem::path>();
                    }
                    filesByExtension[extension].emplace_back(entry.path());
                }
            }
        }
        return filesByExtension;
    }
}
