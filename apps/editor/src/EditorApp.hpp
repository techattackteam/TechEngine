#pragma once

#include <TechEngine/clientapp/ClientApp.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/LooseSource.hpp>
#include <project/Project.hpp>
#include <ui/ImGuiBridge.hpp>

#include <cstddef>
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

    public:
        EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder);

        ~EditorApp() override;

        const AssetIndex& getAssetIndex() const;

    protected:
        void init() override;

        void configureSimulation() override;

        std::error_code simulationStartupLoad(Scene& scene) override;

        void mainThreadUpdate() override;

        void shutdown() override;

    private:
        void createImGuiContext();

        void initImGuiPlatform();

        void buildImGuiFrame();

        void stopImGui();

        void createStartupScene(Scene& scene);

        ResourceId writeStartupScene(Scene& scene);

        ResourceId findOrCreateTriangleMesh();

        ResourceId createAsset(std::string_view path, StringId typeTag, Side side, const std::function<void(ResourceId, std::vector<std::byte>&)>& write);

        void requireNewAssetPath(std::string_view path) const;
    };
}
