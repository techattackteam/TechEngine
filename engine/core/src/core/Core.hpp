#pragma once

#include <complex.h>

#include "files/PathsBank.hpp"
#include "project/ProjectManager.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class CORE_DLL Core {
    public:
        AppType appType;
        SystemsRegistry m_systemRegistry;
        std::filesystem::path m_rootPath;

    public:
        Core(AppType appType);

        virtual ~Core() = default;

        virtual void init(const std::filesystem::path& rootPath, std::unordered_map<ProjectConfig, std::string>& projectConfigs);

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();
    };
}
