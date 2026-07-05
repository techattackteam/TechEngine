#pragma once

#include "TechEngine/core/fileSystem/IFileSystem.hpp"
#include "TechEngine/core/fileSystem/IFileWatcher.hpp"
#include "Panel.hpp"
#include "fileSystem/FIleTypeRegistry.hpp"
#include "fileSystem/FileWatcher.hpp"
#include "fileSystem/IconsRegistry.hpp"
#include "fileSystem/ResourcePathIndex.hpp"

struct GLFWwindow;

namespace TechEngine {
    class PanelsManager;
    class Client;
    class Server;
    class SystemsRegistry;

    enum class FileType {
        Client,
        Server,
        Common
    };

    class ContentBrowserPanel : public Panel {
    private:
        SystemsRegistry& m_editorRegistry;
        SystemsRegistry& m_clientRegistry;
        SystemsRegistry& m_serverRegistry;
        PanelsManager& m_panelsManager;
        FileWatcher* m_fileWatcher;
        TextureLoader* m_textureLoader;
        MaterialLoader* m_materialLoader;
        MeshLoader* m_meshLoader;
        ModelLoader* m_modelLoader;
        SceneLoader* m_sceneLoader;
        IFileSystem& m_fileSystem;

        FIleTypeRegistry m_fileTypeRegistry;
        IconRegistry m_iconRegistry;
        ResourcePathIndex m_pathIndex;

        struct FileNode {
            std::filesystem::path virtualPath;
            std::string label;
            bool isDirectory = false;
            bool hasSubDirs = false;
            IconEntry icon;
            glm::vec2 iconSize = glm::vec2(0.f);
            std::vector<FileNode> children;
        };

        struct CellLayout {
            float iconSize;
            float cellPadding;
            float columnWidth;
            float textMaxWidth;
            int maxTextLines;
        };

        FileNode m_rootNode;
        const FileNode* m_currentNode;

        std::filesystem::path m_currentPath;
        std::unordered_set<std::filesystem::path> m_selectedPaths;
        std::filesystem::path m_anchorPath; // for shift + click

        std::vector<FileNode> m_currentDirEntries;

        uint32_t m_watcherToken = 0;
        bool m_dirtyCache = true;
        float m_iconSize = 64.f;
        float m_iconPadding = 16.f;

        // Marquee state
        bool m_isDragging = false;
        ImVec2 m_dragStart = {0, 0};
        ImVec2 m_dragEnd = {0, 0};
        bool m_dragStartedOnEmpty = false;

        // External (OS) drag & drop state
        struct DropZone {
            ImVec2 min = {0, 0};
            ImVec2 max = {0, 0};
            std::filesystem::path virtualPath;
        };

        static inline ContentBrowserPanel* s_dropTarget = nullptr;
        std::vector<std::filesystem::path> m_pendingExternalDrops;
        std::vector<DropZone> m_folderDropZones;
        ImVec2 m_externalDropPos = {0, 0};
        ImVec2 m_panelMin = {0, 0};
        ImVec2 m_panelMax = {0, 0};
        bool m_hasExternalDrop = false;

    public:
        ContentBrowserPanel(SystemsRegistry& editorRegistry,
                            SystemsRegistry& clientRegistry,
                            SystemsRegistry& serverRegistry,
                            PanelsManager& panelsManager,
                            TextureLoader* m_textureLoader,
                            MaterialLoader* m_materialLoader,
                            MeshLoader* m_meshLoader,
                            ModelLoader* m_modelLoader,
                            SceneLoader* m_sceneLoader,
                            FileWatcher* fileWatcher);

        void onInit();

        void prebuildIconsCache();

        void prebuildThumbnails();

        void onUpdate() override;

        void openFolderOrFile(const std::filesystem::path& filename);

        void renameFile(const std::filesystem::path& filename, const std::string& newName);

        void deleteDirectory(const std::filesystem::path& virtualPath);

        void deleteFile(const std::filesystem::path& virtualPath) const;

        void deleteSelectedItems();

        void renderDirectoryHierarchy(const FileNode& node);

        void renderBreadcrumbs();

        void renderFiles();

        bool openMenuPopupItem(const std::filesystem::path& virtualPath);

        bool openMenuPopupWindow(const std::filesystem::path& virtualPath);

        bool openMenuPopupForFolderHierarchy(const std::filesystem::path& virtualPath);

        void runForFileType(std::filesystem::path path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction);

        void validateCurrentPath();

    private:
        void setupFileTypeRegistryHandlers();

        void rebuildCache();

        void renderSettingsBar();

        std::vector<std::string> computeWrappedLines(const std::string& label, float maxWidth, int maxLines) const;

        IconEntry resolveDisplayTexture(const FileNode& child);

        bool renderFileItem(const FileNode& child, int childIndex, const CellLayout& layout);

        FileNode buildFileNode(const std::filesystem::path& virtualPath);

        const FileNode* findNode(const FileNode& node, std::filesystem::path virtualPath);

        void unregisterNodeResources(const FileNode& node);

        void unregisterResourceForFile(const std::filesystem::path& virtualPath);

        bool isResourceRegisteredForFile(const std::filesystem::path& virtualPath);

        static bool isResourceExtension(const std::string& extension);

        void buildResourcePathIndex();

        void handleFileWatcherEvent(const FileChangedEvent& event);

        void collectModelDependencyPaths(const std::filesystem::path& modelVirtualPath, std::vector<std::filesystem::path>& outPaths);

        void deletePhysicalPathsAsync(std::vector<std::filesystem::path> virtualPaths);

        bool ensureResourceLoaded(const std::filesystem::path& virtualPath);

        void loadResourcesUnder(const std::filesystem::path& virtualFolder);

        FileType getFileType(const std::filesystem::path& virtualPath);

        bool isSelected(const std::filesystem::path& virtualPath) const;

        void handleItemClick(const std::filesystem::path& virtualPath, int childIndex);

        void rangeSelect(int from, int to, bool additive);

        CellLayout makeCellLayout(float iconSize, float iconPadding);

        static void onDropCallback(GLFWwindow* window, int count, const char** paths);

        void processExternalDrop();

        void importExternalPaths(const std::vector<std::filesystem::path>& osPaths,
                                 const std::filesystem::path& targetVirtualDir);

        std::filesystem::path makeUniquePhysicalPath(const std::filesystem::path& desired) const;
    };
};
