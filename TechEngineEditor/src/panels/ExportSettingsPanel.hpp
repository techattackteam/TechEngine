#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "renderer/RendererSettings.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    class PanelsManager;

    class ExportSettingsPanel : public Panel {
    private:
        int32_t width = RendererSettings::width;
        int32_t height = RendererSettings::height;

        bool visible = false;

        PanelsManager &panelsManager;
        ProjectManager &projectManager;
        SceneManager &sceneManager;

    public:
        explicit ExportSettingsPanel(PanelsManager &panelsManager, ProjectManager &projectManager, SceneManager &sceneManager);

        ~ExportSettingsPanel() override;

        void onUpdate() override;

        void exportProject();

        void serializeEngineSettings(const std::filesystem::path &exportPath);

        void setVisibility(bool visible);

        bool isVisible();
    };
}
