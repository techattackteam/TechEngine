#pragma once

#include "Shader.hpp"

namespace TechEngine {
    class ShadersManager {
    private:
        std::unordered_map<std::string, Shader*> shaders;
        Shader* activeShader;

    public:
        ShadersManager();

        ~ShadersManager();

        void init(const std::string& resourcesPath);

        void changeActiveShader(const std::string& name);

        Shader* getActiveShader();

        void unBindShader();


        void exportShaderFiles(const std::string& path);
    };
}
