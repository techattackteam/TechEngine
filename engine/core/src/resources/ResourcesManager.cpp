#include "ResourcesManager.hpp"


#include "files/FileUtils.hpp"
#include "project/Project.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ResourcesManager::ResourcesManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void ResourcesManager::init(AppType appType) {
        std::unordered_map<std::string, std::vector<std::filesystem::path>> filesByExtension = getFilesByExtension(appType);

        m_materialManager.init(filesByExtension[".mat"]);
        m_meshManager.init(filesByExtension[".tesmesh"]);
    }

    void ResourcesManager::shutdown() {
        System::shutdown();
        //m_materialManager.shutdown();
        m_meshManager.shutdown();
    }
#pragma region MeshManager
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
#pragma endregion

#pragma region MaterialManager
    void ResourcesManager::createMaterial(const std::string& name, const std::string& path) {
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
