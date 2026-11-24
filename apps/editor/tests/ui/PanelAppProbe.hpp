#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorCommands.hpp>

#include <vector>

namespace TechEngineTests {
    class PanelAppProbe final : public TechEngine::App {
    public:
        // Never run, so a command a panel sends is refused and logged.
        TechEngine::EditSession session;
        TechEngine::AssetIndex assets;
        std::vector<TechEngine::BatchId> importBatches;
        TechEngine::EditorCommands commands{*this, session, m_registry, m_files, m_resources, m_sceneHandle, assets, m_jobs, importBatches};

        PanelAppProbe() : App(TechEngine::Role::DedicatedServer) {
        }

    protected:
        void init() override {
        }
    };
}
