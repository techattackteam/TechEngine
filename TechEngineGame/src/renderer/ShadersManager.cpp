#include "ShadersManager.hpp"

ShadersManager::ShadersManager() {

}

ShadersManager::~ShadersManager() {
    for (auto &element: shaders) {
        delete (element.second);
    }
}

void ShadersManager::init() {
    shaders = std::unordered_map<std::string, Shader *>();
    shaders["geometry"] = new Shader("geometry", "../TechEngineEditor/resources/shaders/geometryVertex.shader", "../TechEngineEditor/resources/shaders/geometryFragment.shader");;
    shaders["shadowMap"] = new Shader("shadowMap", "../TechEngineEditor/resources/shaders/shadowMapVertex.shader", "../TechEngineEditor/resources/shaders/shadowMapFragment.shader");
    shaders["depthDebug"] = new Shader("depthDebug", "../TechEngineEditor/resources/shaders/debugDepthVertex.shader", "../TechEngineEditor/resources/shaders/debugDepthFragment.shader");
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
