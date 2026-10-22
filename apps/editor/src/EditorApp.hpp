#pragma once

#include <TechEngine/clientapp/ClientApp.hpp>

#include <assets/AssetIndex.hpp>
#include <project/Project.hpp>

#include <filesystem>
#include <system_error>

namespace TechEngine {
    class EditorApp : public ClientApp {
    private:
        std::filesystem::path m_projectRoot;
        std::filesystem::path m_exportFolder;
        Project m_project;
        AssetIndex m_assets;

    public:
        EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder);

        ~EditorApp() override = default;

        const AssetIndex& getAssetIndex() const;

    protected:
        void init() override;

        std::error_code simulationStartupLoad(Scene& scene) override;

    private:
        void createStartupScene(Scene& scene);

        ResourceId writeStartupScene(Scene& scene);
    };
}
