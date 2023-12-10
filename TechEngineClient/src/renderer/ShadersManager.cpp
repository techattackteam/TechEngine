#include <filesystem>
#include "ShadersManager.hpp"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"

namespace TechEngine {


    ShadersManager::ShadersManager() {

    }

    ShadersManager::~ShadersManager() {
        for (auto &element: shaders) {
            delete (element.second);
        }
    }

    void ShadersManager::init() {
        shaders = std::unordered_map<std::string, Shader *>();
        shaders["geometry"] = new Shader("geometry", "resources/shaders/geometryVertex.glsl", "resources/shaders/geometryFragment.glsl");
        shaders["shadowMap"] = new Shader("shadowMap", "resources/shaders/shadowMapVertex.glsl", "resources/shaders/shadowMapFragment.glsl");
        shaders["debugDepth"] = new Shader("depthDebug", "resources/shaders/debugDepthVertex.glsl", "resources/shaders/debugDepthFragment.glsl");
        shaders["line"] = new Shader("line", "resources/shaders/lineVertex.glsl", "resources/shaders/lineFragment.glsl");
    }

    void ShadersManager::changeActiveShader(const std::string &name) {
        if (activeShader != nullptr) {
            activeShader->unBind();
        }
        activeShader = shaders[name];
        activeShader->bind();
    }

    void ShadersManager::unBindShader() {
        activeShader->unBind();
    }

    Shader *ShadersManager::getActiveShader() {
        return activeShader;
    }

    void ShadersManager::exportShaderFiles(const std::string &path) {
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
        for (auto &element: shaders) {
            std::string newPath = FileSystem::editorResourcesPath.string() + "/shaders/" + element.first + "Vertex.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception &e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Vertex.glsl", newPath, e.what());
            }
            newPath = FileSystem::editorResourcesPath.string() + "/shaders/" + element.first + "Fragment.glsl";
            try {
                std::filesystem::copy(newPath, path);
            } catch (std::exception &e) {
                TE_LOGGER_ERROR("Failed to copy Shader {0} into: {1}. \n {2}", element.first + "Fragment.glsl", newPath, e.what());
            }
        }
    }
}