#pragma once

#include <vector>
#include "Shader.hpp"

namespace TechEngine {

    class ShadersManager {
    private:
        std::unordered_map<std::string, Shader *> shaders;
        Shader *activeShader;

    public:

        ShadersManager();

        ~ShadersManager();

        void changeActiveShader(const std::string &name);

        Shader *getActiveShader();

        void unBindShader();

        void init();

        void exportShaderFiles(const std::string &path);
    };
}
