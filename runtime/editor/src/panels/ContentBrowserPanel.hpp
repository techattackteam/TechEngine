#pragma once
#include <filesystem>

#include "Panel.hpp"


namespace TechEngine {
    class PanelsManager;
    class Client;
    class Server;
    class SystemsRegistry;

    enum class FileType {
        CLIENT_FILE,
        SERVER_FILE,
        COMMON_FILE
    };

    class ContentBrowserPanel : public Panel {
    private:
        std::filesystem::path m_currentPath;
        std::filesystem::path m_selectedPath;

        SystemsRegistry& m_editorRegistry;
        PanelsManager& m_panelsManager;

    public:
        ContentBrowserPanel(SystemsRegistry& editorRegistry, PanelsManager& panelsManager);

        void onInit();

        void onUpdate() override;

        void openFolderOrFile(const std::string& filename);

        void renameFile(const std::string& filename, const std::string& newName);

        void tryLoadModel(const std::string& filepath);

        void deleteFile(const std::string& filename);

        void renderDirectoryHierarchy(const std::filesystem::path& directoryPath);

        void renderDirectoryHierarchyRecurse(const std::filesystem::path& path);

        void renderFiles(const std::filesystem::path& directoryPath);

        bool openMenuPopupItem(const std::filesystem::path& path);

        bool openMenuPopupWindow(const std::filesystem::path& path);

        void runFunctionBasedOnFileType(std::string path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction);

    private:
        FileType getFileType(const std::filesystem::path& path);
    };
};
