#pragma once


#include "systems/SystemsRegistry.hpp"
#include <filesystem>

namespace TechEngine {
    enum class AppType;

    class CORE_DLL Core {
    public:
        SystemsRegistry m_systemRegistry;
        std::filesystem::path m_rootPath;

    public:
        virtual ~Core() = default;

        virtual void registerSystems(const std::filesystem::path& rootPath);

        virtual void init(AppType appType);

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();
    };
}
