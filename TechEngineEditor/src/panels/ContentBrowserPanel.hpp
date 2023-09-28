#pragma once

#include <filesystem>
#include "Panel.hpp"

namespace TechEngine {
    class ContentBrowserPanel : public Panel {
    private:
        std::filesystem::path currentPath;
        bool menuWindowOpen = false;
    public:
        ContentBrowserPanel();

        void init();

        void onUpdate() override;

        void openFile(const std::string &filename);

        void renameFile(const std::string &filename, const std::string &newName);

        void deleteFile(const std::string &filename);

        bool Splitter(bool split_vertically, float thickness, float *size1, float *size2, float min_size1, float min_size2, float splitter_long_axis_size);

        void RenderSceneHierarchy(const std::filesystem::path &directoryPath);
    };
}
