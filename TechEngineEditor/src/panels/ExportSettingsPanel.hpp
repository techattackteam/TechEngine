#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "renderer/RendererSettings.hpp"

namespace TechEngine {
    class ExportSettingsPanel : public Panel {
    private:
        int32_t width = RendererSettings::width;
        int32_t height = RendererSettings::height;

        const std::string &currentDirectory;
        const std::string &projectDirectory;
        const std::string &buildDirectory;
        const std::string &cmakeProjectDirectory;
        std::string &currentScenePath;

        bool visible = false;

    public:
        ExportSettingsPanel(const std::string &currentDirectory,
                            const std::string &projectDirectory,
                            const std::string &buildDirectory,
                            const std::string &cmakeProjectDirector,
                            std::string &currentScenePath);

        ~ExportSettingsPanel() override;

        void onUpdate() override;

        void exportProject();

        void serializeEngineSettings(const std::filesystem::path &exportPath);

        void setVisibility(bool visible);

        bool isVisible();
    };
}
