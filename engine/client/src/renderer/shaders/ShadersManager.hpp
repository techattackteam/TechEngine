#pragma once

#include <filesystem>

#include "Shader.hpp"

namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    struct ShaderStageInfo {
        ShaderType m_type;
        std::filesystem::path m_path;
    };

    struct ShaderInfo {
        std::string name;
        std::vector<ShaderStageInfo> stages;
    };

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
    };
}
