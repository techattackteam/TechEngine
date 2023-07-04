#include "ProjectManager.hpp"

ProjectManager::ProjectManager() {

}

ProjectManager *ProjectManager::getInstance() {
    if (instance == nullptr) {
        instance = new ProjectManager();
    }
    return instance;
}

const path &ProjectManager::getRootPath() {
    return getInstance()->rootPath;
}

const path &ProjectManager::getUserProjectRootPath() {
    return getInstance()->userProjectRootPath;
}

const path &ProjectManager::getUserProjectScriptsPath() {
    return getInstance()->userProjectScriptsPath;
}

const path &ProjectManager::getUserProjectScenePath() {
    return getInstance()->userProjectScenesPath;
}

const path &ProjectManager::getUserScriptsDLLPath() {
    return getInstance()->userScriptsDLLPath;
}

const path &ProjectManager::getEngineExportSettingsFile() {
    return getInstance()->engineExportSettingsFile;
}

const path &ProjectManager::getRuntimePath() {
    return getInstance()->runtimePath;
}

const path &ProjectManager::getBuildPath() {
    return getInstance()->buildPath;
}

const path &ProjectManager::getBuildResourcesPath() {
    return getInstance()->buildResourcesPath;
}

const path &ProjectManager::getResourcesPath() {
    return getInstance()->resourcesPath;
}

const path &ProjectManager::getUserProjectBuildPath() {
    return getInstance()->userProjectBuildPath;
}

const path &ProjectManager::getCmakePath() {
    return getInstance()->cmakePath;
}
