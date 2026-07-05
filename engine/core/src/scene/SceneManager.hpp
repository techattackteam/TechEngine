#pragma once

#include "systems/System.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    class SystemsRegistry;

    class CORE_DLL SceneManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Scene m_activeScene;

    public:
        explicit SceneManager(SystemsRegistry& systemsRegistry);

        void init();

        void shutdown() override;

        Scene& getActiveScene();

        const std::string& getActiveSceneName() const;

        void clear();
    };
}
