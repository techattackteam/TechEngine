#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/core/scene/Scene.hpp>

#include <system_error>

namespace TechEngineTests {
    class AppTestAccess {
    public:
        static TechEngine::Scene& scene(TechEngine::App& app) {
            return app.m_scene;
        }

        static const TechEngine::Scene& scene(const TechEngine::App& app) {
            return app.m_scene;
        }

        static std::error_code simulationStartupLoad(TechEngine::App& app) {
            return app.simulationStartupLoad(app.m_scene);
        }
    };
}
