#pragma once

#include <vector>
#include "Shader.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    class ShadersManager {
    private:
        std::unordered_map<std::string, Shader*> shaders;
        Shader* activeShader;

    public:
        ShadersManager();

        ~ShadersManager();

        void init(ProjectManager& projectManager);

        void changeActiveShader(const std::string& name);

        Shader* getActiveShader();

        void unBindShader();


        void exportShaderFiles(const std::string& path);
    };
}
