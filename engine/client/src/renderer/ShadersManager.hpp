#pragma once

#include "Shader.hpp"

namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class ShadersManager {
    private:
        SystemsRegistry& m_systemsRegistry;

        std::unordered_map<std::string, Shader*> shaders;
        Shader* activeShader;

    public:
        ShadersManager(SystemsRegistry& systemsRegistry);

        ~ShadersManager();

        void init();

        void changeActiveShader(const std::string& name);

        Shader* getActiveShader();

        void unBindShader();


        void exportShaderFiles(const std::string& path);
    };
}
