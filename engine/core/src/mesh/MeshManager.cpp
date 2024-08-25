#include "MeshManager.hpp"

#include <filesystem>
#include <fstream>

#include "Mesh.hpp"
#include "serialization/FileStream.hpp"

namespace TechEngine {
    void MeshManager::init() {
        //m_meshesBank.clear();
    }


    void MeshManager::shutdown() {
        m_meshesBank.clear();
    }

    void MeshManager::registerMesh(const std::filesystem::path& path) {
        std::string name = path.stem().string();
        if (isMeshRegistered(name)) {
            return;
        }
        Mesh mesh = loadStaticMesh(path);

        m_meshesBank[name] = mesh;
    }

    const Mesh& MeshManager::getMesh(const std::string& name) {
        return m_meshesBank[name];
    }

    Mesh MeshManager::loadStaticMesh(const std::filesystem::path& path) {
        FileStreamReader reader(path);
        Mesh mesh;
        mesh.m_name = path.stem().string();
        reader.readObject(mesh);
        return mesh;
    }

    bool MeshManager::isMeshRegistered(const std::string& name) {
        return m_meshesBank.find(name) != m_meshesBank.end();
    }
}
