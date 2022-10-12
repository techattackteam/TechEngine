#pragma once

#include <filesystem>
#include "Panel.hpp"

namespace TechEngine {
    class ProjectBrowserPanel : public Panel {
    private:
        std::filesystem::path projectPath;
        std::filesystem::path currentPath;
    public:
        ProjectBrowserPanel();


        void onUpdate() override;
    };
}
