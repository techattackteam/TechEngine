#pragma once

#include <filesystem>
#include "Panel.hpp"

namespace TechEngine {
    class ProjectBrowserPanel : public Panel {
    private:
        std::filesystem::path currentPath;
        bool menuWindowOpen = false;
    public:
        ProjectBrowserPanel();

        void init();

        void onUpdate() override;

        void renameFile(const std::string &filename, const std::string &newName);

        void openFile(const std::string &filename);
    };
}
