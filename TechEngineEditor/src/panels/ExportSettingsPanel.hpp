#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "renderer/RendererSettings.hpp"

namespace TechEngine {
    class ExportSettingsPanel : public Panel {
    private:
        int32_t width = RendererSettings::width;
        int32_t height = RendererSettings::height;

        std::string &currentScenePath;

        bool visible = false;

    public:
        explicit ExportSettingsPanel(std::string &currentScenePath);

        ~ExportSettingsPanel() override;

        void onUpdate() override;

        void exportProject();

        void serializeEngineSettings(const std::filesystem::path &exportPath);

        void setVisibility(bool visible);

        bool isVisible();
    };
}
