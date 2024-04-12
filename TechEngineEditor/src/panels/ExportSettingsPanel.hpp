#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "project/ProjectManager.hpp"
#include "renderer/ShadersManager.hpp"

namespace TechEngine {
    class PanelsManager;

    class ExportSettingsPanel : public Panel {
    private:
        int32_t width = 1080;
        int32_t height = 720;

        PanelsManager& panelsManager;
        ProjectManager& projectManager;
        SceneManager& sceneManager;
        ShadersManager& shadersManager;

    public:
        explicit ExportSettingsPanel(PanelsManager& panelsManager, ProjectManager& projectManager, SceneManager& sceneManager, ShadersManager& shadersManager);

        ~ExportSettingsPanel() override;

        void onUpdate() override;

        void exportProject();

        void serializeEngineSettings(const std::filesystem::path& exportPath);
    };
}
