#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>

#include <assets/AssetIndex.hpp>
#include <project/Project.hpp>

#include <filesystem>
#include <string>

namespace TechEngine {
    class EditorApp : public App {
    private:
        std::filesystem::path m_projectRoot;
        Project m_project;
        AssetIndex m_assets;
        Client m_client;
        std::string m_appliedTitle;

    public:
        explicit EditorApp(std::filesystem::path projectRoot);
        ~EditorApp() override = default;
        static Role editorRole();

        const AssetIndex& getAssetIndex() const;

    protected:
        void init() override;

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        void wakeMainThread() override;

        bool renderTiming(RenderTiming& out) const override;

        void shutdown() override;

        bool shouldClose() const override;
    };
}
