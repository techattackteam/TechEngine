#pragma once

#include <TechEngine/clientapp/ClientApp.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/LooseSource.hpp>
#include <edit/EditSession.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/EditorSnapshotSlot.hpp>
#include <project/Project.hpp>
#include <ui/ClosePrompt.hpp>
#include <ui/ImGuiBridge.hpp>
#include <ui/PanelsManager.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>
#include <system_error>
#include <vector>

namespace TechEngine {
    class EditorApp : public ClientApp {
    private:
        std::filesystem::path m_projectRoot;
        std::filesystem::path m_exportFolder;
        Project m_project;
        AssetIndex m_assets;
        LooseSource m_source;
        ImGuiBridge m_imguiBridge;
        // Simulation thread only: commands and publishEdits read and write it there.
        EditSession m_session;
        std::uint64_t m_loggedEditRevision = 0;
        EditorSnapshotSlot m_snapshotSlot;
        // Main thread only.
        EditorSnapshot m_snapshot;
        EditorCommands m_commands;
        PanelsManager m_panels;
        ClosePrompt m_closePrompt;

    public:
        EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder);

        ~EditorApp() override;

        const AssetIndex& getAssetIndex() const;

    protected:
        void init() override;

        void configureSimulation() override;

        bool simulationTicks() const override;

        std::error_code simulationStartupLoad(Scene& scene) override;

        void mainThreadUpdate() override;

        void publishEdits(const Scene& scene) override;

        bool takeSnapshot(EditorSnapshot& out);

        void logSelectedTransform(const Scene& scene, const EditorSnapshot& snapshot);

        // Simulation thread only, or before the simulation starts.
        EditSession& getSession();

        void shutdown() override;

    private:
        void createImGuiContext();

        void initImGuiPlatform();

        void loadLayout();

        void saveLayout();

        void handleCloseRequest();

        void buildImGuiFrame();

        void updateTitle();

        void drawClosePrompt();

        void stopImGui();

        void createStartupScene(Scene& scene);

        ResourceId writeStartupScene(Scene& scene);

        ResourceId findOrCreateTriangleMesh();

        ResourceId createAsset(std::string_view path, StringId typeTag, Side side, const std::function<void(ResourceId, std::vector<std::byte>&)>& write);

        void requireNewAssetPath(std::string_view path) const;
    };
}
