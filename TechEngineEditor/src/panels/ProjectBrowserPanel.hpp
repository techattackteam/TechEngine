#pragma once

#include <filesystem>
#include "Panel.hpp"

namespace TechEngine {
    class ProjectBrowserPanel : public Panel {
    private:
        std::filesystem::path currentPath;
    public:
        ProjectBrowserPanel();

        void init();

        void onUpdate() override;
    };
}
