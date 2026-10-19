#pragma once

#include <TechEngine/app/App.hpp>
#include <TechEngine/client/Client.hpp>

#include <assets/AssetIndex.hpp>
#include <project/Project.hpp>

#include <filesystem>
#include <string>
#include <system_error>

namespace TechEngine {
    class EditorApp : public App {
    private:
        std::filesystem::path m_projectRoot;
        std::filesystem::path m_exportFolder;
        Project m_project;
        AssetIndex m_assets;
        Client m_client;
        std::string m_appliedTitle;

    public:
        EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder);

        ~EditorApp() override = default;

        static Role editorRole();

        const AssetIndex& getAssetIndex() const;

    protected:
        void init() override;

        std::error_code simulationStartupLoad(Scene& scene) override;

        void publishSnapshot(const SimulationContext& simulation) override;

        void mainThreadUpdate() override;

        void wakeMainThread() override;

        bool renderTiming(RenderTiming& out) const override;

        void shutdown() override;

        bool shouldClose() const override;

    private:
        void createStartupScene(Scene& scene);

        ResourceId writeStartupScene(Scene& scene);
    };
}
