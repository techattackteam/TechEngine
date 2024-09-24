
#include "ShadersManager.hpp"
#include "files/FileUtils.hpp"
#include "core/Logger.hpp"
#include "project/Project.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    ShadersManager::ShadersManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    ShadersManager::~ShadersManager() {
        for (auto& element: shaders) {
            delete (element.second);
        }
    }

    void ShadersManager::init() {
        shaders = std::unordered_map<std::string, Shader*>();
        std::string clientPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string();
        shaders["geometry"] = new Shader("geometry", (clientPath + "/assets/shaders/geometryVertex.glsl").c_str(),
                                         (clientPath + "/assets/shaders/geometryFragment.glsl").c_str());
        shaders["line"] = new Shader("line", (clientPath + "/assets/shaders/lineVertex.glsl").c_str(),
                                     (clientPath + "/assets/shaders/lineFragment.glsl").c_str());
    }

    void ShadersManager::changeActiveShader(const std::string& name) {
        if (activeShader != nullptr) {
            activeShader->unBind();
        }
        activeShader = shaders[name];
        activeShader->bind();
    }

    void ShadersManager::unBindShader() {
        activeShader->unBind();
    }

    Shader* ShadersManager::getActiveShader() {
        return activeShader;
    }

    void ShadersManager::exportShaderFiles(const std::string& path) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
        for (auto& element: shaders) {
            std::string newPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string() + "/shaders/" + element.first + "Vertex.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Vertex.glsl", newPath, e.what());
            }
            newPath = m_systemsRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Client).string() + "/shaders/" + element.first + "Fragment.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Fragment.glsl", newPath, e.what());
            }
        }
    }
}
