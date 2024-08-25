#pragma once

#include "files/PathsBank.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class Core {
    public:
        SystemsRegistry m_systemRegistry;
        std::filesystem::path m_rootPath;

    public:
        virtual ~Core() = default;

        virtual void init(const std::filesystem::path& rootPath);

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onStop();

        virtual void shutdown();
    };
}
