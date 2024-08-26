#include "ResourcesManager.hpp"

namespace TechEngine {
    void ResourcesManager::init() {
        System::init();
        //m_materialManager.init();
        m_meshManager.init();
    }

    void ResourcesManager::shutdown() {
        System::shutdown();
        //m_materialManager.shutdown();
        m_meshManager.shutdown();
    }

    void ResourcesManager::loadModelFile(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        std::string parentFolder = std::filesystem::path(path).parent_path().string();
        std::string staticMeshPath = parentFolder + "\\" + modelName + ".TE_mesh";
        AssimpLoader::Node node = m_assimpLoader.loadModel(path);
        createStaticMeshFile(node, staticMeshPath);
        registerStaticMesh(staticMeshPath);
    }

    void ResourcesManager::registerStaticMesh(const std::string& path) {
        std::string modelName = FileUtils::getFileName(path);
        if (m_meshManager.isMeshRegistered(modelName)) {
            TE_LOGGER_WARN("Mesh already registered: {0}", path);
            return;
        }
        AssimpLoader::MeshData mesh = loadStaticMesh(path);
        m_meshManager.registerMesh(modelName, mesh, m_materialManager.createMaterial("default", glm::vec4(1.0f), glm::vec3(0.1f), glm::vec3(0.5f), glm::vec3(0.5f), 32.0f));
    }

    std::filesystem::path ResourcesManager::createStaticMeshFile(const AssimpLoader::Node& node, const std::string& staticMeshPath) {
        std::queue<AssimpLoader::Node> nodeQueue;
        nodeQueue.push(node);

        int lastIndex = 0;
        FileStreamWriter writer(staticMeshPath);
        while (!nodeQueue.empty()) {
            // Get the current node
            AssimpLoader::Node currentNode = nodeQueue.front();
            nodeQueue.pop();

            // Add current node's meshes to the list
            for (AssimpLoader::MeshData& meshData: currentNode.meshes) {
                writer.writeArray(meshData.vertices);

                std::vector<int> indices;
                for (int index: meshData.indices) {
                    indices.push_back(index + lastIndex);
                }
                writer.writeArray(meshData.indices);
                lastIndex = meshData.indices.back();
            }

            // Add current node's children to the queue
            for (const AssimpLoader::Node& child: currentNode.children) {
                nodeQueue.push(child);
            }
        }

        return staticMeshPath;
    }

    AssimpLoader::MeshData ResourcesManager::loadStaticMesh(const std::filesystem::path& path) {
        FileStreamReader reader(path);
        AssimpLoader::MeshData mesh;
        reader.readObject(mesh);
        return mesh;
    }
}
