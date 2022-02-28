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
    shaders["geometry"] = new Shader("geometry", "../engine/res/shaders/geometryVertex.shader", "../engine/res/shaders/geometryFragment.shader");;
    shaders["shadowMap"] = new Shader("shadowMap", "../engine/res/shaders/shadowMapVertex.shader", "../engine/res/shaders/shadowMapFragment.shader");
    shaders["depthDebug"] = new Shader("depthDebug", "../engine/res/shaders/debugDepthVertex.shader", "../engine/res/shaders/debugDepthFragment.shader");
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
