#include <filesystem>
#include "ShadersManager.hpp"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    ShadersManager::ShadersManager() {
    }

    ShadersManager::~ShadersManager() {
        for (auto& element: shaders) {
            delete (element.second);
        }
    }

    void ShadersManager::init(const std::string& resourcesPath) {
        shaders = std::unordered_map<std::string, Shader*>();
        shaders["geometry"] = new Shader("geometry", (resourcesPath + "/defaults/shaders/geometryVertex.glsl").c_str(),
                                         (resourcesPath + "/defaults/shaders/geometryFragment.glsl").c_str());

        shaders["shadowMap"] = new Shader("shadowMap", (resourcesPath + "/defaults/shaders/shadowMapVertex.glsl").c_str(),
                                          (resourcesPath + "/defaults/shaders/shadowMapFragment.glsl").c_str());

        shaders["debugDepth"] = new Shader("depthDebug", (resourcesPath + "/defaults/shaders/debugDepthVertex.glsl").c_str(),
                                           (resourcesPath + "/defaults/shaders/debugDepthFragment.glsl").c_str());

        shaders["line"] = new Shader("line", (resourcesPath + "/defaults/shaders/lineVertex.glsl").c_str(),
                                     (resourcesPath + "/defaults/shaders/lineFragment.glsl").c_str());
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
            std::string newPath = FileSystem::editorResourcesPath.string() + "/shaders/" + element.first + "Vertex.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Vertex.glsl", newPath, e.what());
            }
            newPath = FileSystem::editorResourcesPath.string() + "/shaders/" + element.first + "Fragment.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception& e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Fragment.glsl", newPath, e.what());
            }
        }
    }
}
