#pragma once

#include "Shader.hpp"
#include "core/FilePaths.hpp"

namespace TechEngine {
    class ShadersManager {
    private:
        std::unordered_map<std::string, Shader*> shaders;
        Shader* activeShader;

    public:
        ShadersManager();

        ~ShadersManager();

        void init(FilePaths& filePaths);

        void changeActiveShader(const std::string& name);

        Shader* getActiveShader();

        void unBindShader();


        void exportShaderFiles(const std::string& path);
    };
}
