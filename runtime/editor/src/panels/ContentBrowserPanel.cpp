#include "PanelsManager.hpp"
#include "ContentBrowserPanel.hpp"

#include "core/Logger.hpp"
#include "core/Parallel.hpp"
#include "events/resourcersManager/texture/TextureCreatedEvent.hpp"
#include "eventSystem/EventManager.hpp"
#include "fileSystem/FileWatcher.hpp"
#include "resources/ResourceSystem.hpp"
#include "resources/loaders/MaterialLoader.hpp"
#include "resources/loaders/MeshLoader.hpp"
#include "resources/loaders/ModelLoader.hpp"
#include "resources/loaders/SceneLoader.hpp"
#include "scene/SceneManager.hpp"
#include "UIUtils/ImGuiUtils.hpp"
#include "window/Window.hpp"

#include <thread>

namespace TechEngine {
    ContentBrowserPanel::ContentBrowserPanel(SystemsRegistry& editorRegistry,
                                             SystemsRegistry& clientRegistry,
                                             SystemsRegistry& serverRegistry,
                                             PanelsManager& panelsManager,
                                             TextureLoader* textureLoader,
                                             MaterialLoader* materialLoader,
                                             MeshLoader* assimpLoader,
                                             ModelLoader* modelLoader,
                                             SceneLoader* sceneLoader,
                                             FileWatcher* fileWatcher) : m_editorRegistry(editorRegistry),
                                                                         m_clientRegistry(clientRegistry),
                                                                         m_serverRegistry(serverRegistry),
                                                                         m_panelsManager(panelsManager),
                                                                         m_fileSystem(editorRegistry.getSystem<FileSystem>()),
                                                                         m_iconRegistry(editorRegistry.getSystem<FileSystem>()),
                                                                         m_textureLoader(textureLoader),
                                                                         m_materialLoader(materialLoader),
                                                                         m_meshLoader(assimpLoader),
                                                                         m_modelLoader(modelLoader),
                                                                         m_sceneLoader(sceneLoader),
                                                                         m_fileWatcher(fileWatcher),
                                                                         Panel(editorRegistry) {
    }


    void ContentBrowserPanel::onInit() {
        m_currentPath = "editorAssetsClient://";
        setupFileTypeRegistryHandlers();

        // Route OS (e.g. Windows Explorer) drops on the main window into this panel.
        // ImGui's GLFW backend does not install a drop callback, so this is safe.
        if (GLFWwindow* handle = m_editorRegistry.getSystem<Window>().getHandler()) {
            s_dropTarget = this;
            glfwSetDropCallback(handle, &ContentBrowserPanel::onDropCallback);
        }


        // Subscribe to TextureCreatedEvent so runtime imports get thumbnails too.
        m_clientRegistry.getSystem<EventManager>().subscribe<TextureCreatedEvent>(
            [this](const std::shared_ptr<Event>& event) {
                const auto& e = static_cast<const TextureCreatedEvent&>(*event);
                const auto tex = m_clientRegistry.getSystem<ResourceSystem>()
                        .getTextureResource(e.getUUID());
                if (tex && !m_iconRegistry.hasThumbnail(tex->getUUID())) {
                    m_iconRegistry.buildThumbnailFromPixelData(tex->getUUID(), *tex);
                }
            });


        m_watcherToken = m_fileWatcher->subscribe([this](const FileChangedEvent& event) {
            m_dirtyCache = true;
            handleFileWatcherEvent(event);
        });
        rebuildCache();
        prebuildIconsCache();
        prebuildThumbnails();
        buildResourcePathIndex();
        validateCurrentPath();
    }

    bool ContentBrowserPanel::isResourceExtension(const std::string& extension) {
        return extension == ".temodel" || extension == ".temesh" || extension == ".temat" ||
               extension == ".tetexture" || extension == ".tescene";
    }

    void ContentBrowserPanel::buildResourcePathIndex() {
        // Enumerate every resource file up front, then let ResourcePathIndex read the leading UUID
        // headers in parallel. The heavy work (per-file header reads) happens off the enumeration.
        std::vector<std::filesystem::path> resourceFiles;
        for (const std::filesystem::path& file: m_fileSystem.list("editorAssetsClient://", true)) {
            if (isResourceExtension(file.extension().string())) {
                resourceFiles.push_back(file);
            }
        }
        m_pathIndex.buildParallel(m_fileSystem, resourceFiles);
    }

    void ContentBrowserPanel::prebuildIconsCache() {
        m_iconRegistry.getIcon("editorAssets://icons/folder.png");
        m_iconRegistry.getIcon("editorAssets://icons/default.png");
        m_iconRegistry.getIcon("editorAssets://icons/cpp.png");
        std::vector<std::filesystem::path> files = m_fileSystem.list("editorAssetsClient://", true);
        const std::vector<std::string>& sourceExtensions = m_textureLoader->sourceExtensions();
        for (const std::filesystem::path& file: files) {
            const FileStatus status = m_fileSystem.status(file);
            if (std::ranges::find(sourceExtensions, status.extension) != sourceExtensions.end()) {
                m_iconRegistry.getIcon(status.virtualPath);
            }
        }
    }

    void ContentBrowserPanel::prebuildThumbnails() {
        std::vector<std::filesystem::path> files = m_fileSystem.list("editorAssetsClient://", true);
        for (const std::filesystem::path& file: files) {
            const FileStatus status = m_fileSystem.status(file);
            if (status.extension == m_textureLoader->resourceExtension()) {
                const std::shared_ptr<TextureResource>& textureResource = m_clientRegistry.getSystem<ResourceSystem>().getTextureResource(status.name.stem().string());
                if (textureResource && !m_iconRegistry.hasThumbnail(textureResource->getUUID())) {
                    m_iconRegistry.buildThumbnailFromPixelData(textureResource->getUUID(), *textureResource);
                }
            }
        }
    }

    void ContentBrowserPanel::onUpdate() {
        if (m_dirtyCache) {
            rebuildCache();
            validateCurrentPath();
        }

        m_panelMin = ImGui::GetWindowPos();
        m_panelMax = {m_panelMin.x + ImGui::GetWindowSize().x, m_panelMin.y + ImGui::GetWindowSize().y};
        m_folderDropZones.clear();

        if (ImGui::BeginTable("Content", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Outline", 0, 250.f);
            ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("##Folders", ImVec2(0, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_None);

            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
            renderDirectoryHierarchy(m_rootNode);
            ImGui::PopStyleVar();

            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            renderBreadcrumbs();
            ImGui::BeginChild("##Files", ImVec2(0, ImGui::GetContentRegionAvail().y), false, ImGuiWindowFlags_None);
            renderFiles();
            ImGui::EndChild();
            ImGui::EndTable();
        }

        processExternalDrop();
    }

    void ContentBrowserPanel::renderDirectoryHierarchy(const FileNode& node) {
        // hasSubDirs is already in node — no FS call needed
        ImGuiTreeNodeFlags flags =
                ((m_currentPath == node.virtualPath) ? ImGuiTreeNodeFlags_Selected : 0) |
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanFullWidth |
                (node.hasSubDirs ? 0 : ImGuiTreeNodeFlags_Leaf);

        bool open = ImGui::TreeNodeEx(
            node.virtualPath.generic_string().c_str(),
            flags, "%s", node.label.c_str());

        m_folderDropZones.push_back({ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), node.virtualPath});

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
            m_currentPath = node.virtualPath;
            m_currentNode = &node;
        }

        openMenuPopupForFolderHierarchy(node.virtualPath);

        if (open) {
            for (const FileNode& child: node.children) {
                if (child.isDirectory)
                    renderDirectoryHierarchy(child);
            }
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::renderBreadcrumbs() {
        std::vector<const FileNode*> crumbs;
        std::filesystem::path path = m_currentPath;

        // Walk up via findNode at each ancestor level
        while (!path.empty()) {
            const FileNode* node = findNode(m_rootNode, path);
            if (!node) {
                break;
            }
            crumbs.push_back(node);
            std::filesystem::path parent = m_fileSystem.getParentPath(path);
            if (parent == path) {
                break;
            }
            path = parent;
        }
        std::ranges::reverse(crumbs);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

        for (int i = 0; i < static_cast<int>(crumbs.size()); i++) {
            const FileNode* crumb = crumbs[i];
            const bool isCurrent = (i == static_cast<int>(crumbs.size()) - 1);

            // Use ## to avoid ImGui ID collision between same-named folders at different levels
            const std::string buttonId = crumb->label + "##crumb_" + std::to_string(i);

            if (isCurrent) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
                ImGui::TextUnformatted(crumb->label.c_str());
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.f));
                if (ImGui::SmallButton(buttonId.c_str())) {
                    m_currentPath = crumb->virtualPath;
                    m_currentNode = crumb;
                }
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextDisabled(">");
                ImGui::SameLine();
            }
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    void ContentBrowserPanel::renderFiles() {
        if (!m_currentNode) {
            ImGui::Text("Directory does not exist.");
            return;
        }

        const CellLayout layout = makeCellLayout(m_iconSize, m_iconPadding);

        const float panelW = ImGui::GetContentRegionAvail().x;
        const int columnCount = std::max(1, static_cast<int>(panelW / layout.columnWidth));

        if (!ImGui::BeginTable("##content_grid", columnCount,
                               ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX))
            return;

        bool opened = false;
        int childIndex = 0;

        for (const FileNode& child: m_currentNode->children) {
            ImGui::TableNextColumn();
            ImGui::PushID(child.virtualPath.generic_string().c_str());

            if (renderFileItem(child, childIndex, layout))
                opened = true;

            ImGui::PopID();
            ++childIndex;
        }

        ImGui::EndTable();

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemHovered()) {
            m_selectedPaths.clear();
            m_anchorPath.clear();
        }

        const ImVec2 mousePos = ImGui::GetMousePos();
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
            m_isDragging = true;
            m_dragStartedOnEmpty = true;
            m_dragStart = mousePos;
            m_dragEnd = mousePos;
            if (!ImGui::GetIO().KeyCtrl)
                m_selectedPaths.clear();
            m_anchorPath.clear();
        }

        if (m_isDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_dragEnd = mousePos;
        }

        if (m_isDragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_isDragging = false;
            m_dragStartedOnEmpty = false;
        }

        if (m_isDragging) {
            const ImVec2 rMin = {
                std::min(m_dragStart.x, m_dragEnd.x),
                std::min(m_dragStart.y, m_dragEnd.y)
            };
            const ImVec2 rMax = {
                std::max(m_dragStart.x, m_dragEnd.x),
                std::max(m_dragStart.y, m_dragEnd.y)
            };

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(rMin, rMax, IM_COL32(100, 140, 255, 40));
            dl->AddRect(rMin, rMax, IM_COL32(100, 140, 255, 200), 0.f, 0, 1.5f);
        }


        if (!opened)
            openMenuPopupWindow(m_currentPath);
    }

    void ContentBrowserPanel::openFolderOrFile(const std::filesystem::path& virtualPath) {
        if (m_fileSystem.isDirectory(virtualPath)) {
            m_currentPath = virtualPath;
            m_currentNode = findNode(m_rootNode, virtualPath);
        } else {
            m_fileTypeRegistry.getHandler(virtualPath).handler(virtualPath);
        }
    }

    void ContentBrowserPanel::renameFile(const std::filesystem::path& filename, const std::string& newName) {
        m_fileSystem.renameFile(filename, newName);
    }

    void ContentBrowserPanel::deleteDirectory(const std::filesystem::path& virtualPath) {
        // Cascade the runtime unregistration over the subtree synchronously (main thread), then let
        // the physical removal of the (potentially large) tree run on a background thread.
        const FileNode* node = findNode(m_rootNode, virtualPath);
        if (node) {
            unregisterNodeResources(*node);
        }
        deletePhysicalPathsAsync({virtualPath});
    }

    void ContentBrowserPanel::deleteFile(const std::filesystem::path& virtualPath) const {
        m_fileSystem.deleteFile(virtualPath);
    }

    void ContentBrowserPanel::deleteSelectedItems() {
        std::vector<std::filesystem::path> toDelete(m_selectedPaths.begin(), m_selectedPaths.end());

        for (const std::filesystem::path& path: toDelete) {
            const FileNode* node = findNode(m_rootNode, path);
            if (node) {
                unregisterNodeResources(*node);
            }
        }

        m_fileSystem.deletePaths(toDelete);

        m_selectedPaths.clear();
        m_anchorPath.clear();
    }

    bool ContentBrowserPanel::openMenuPopupItem(const std::filesystem::path& virtualPath) {
        bool opened = false;
        if (ImGui::BeginPopupContextItem()) {
            opened = true;

            if (m_selectedPaths.size() > 1) {
                // Bulk context menu: only offer Delete for now.
                if (ImGui::Button(("Delete " + std::to_string(m_selectedPaths.size()) + " items").c_str())) {
                    deleteSelectedItems();
                    ImGui::CloseCurrentPopup();
                }
            } else {
                // Single-item context menu: delegate to the per-type handler.
                const FileTypeHandler& handler = m_fileTypeRegistry.getHandler(virtualPath);
                handler.onContextMenu(virtualPath);
            }

            ImGui::EndPopup();
        }
        return opened;
    }

    bool ContentBrowserPanel::openMenuPopupWindow(const std::filesystem::path& virtualPath) {
        bool opened = false;
        if (ImGui::BeginPopupContextWindow()) {
            opened = true;
            if (ImGui::BeginMenu("New")) {
                std::string newFolderName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Folder", "New Folder", newFolderName)) {
                    m_fileSystem.createDirectory(virtualPath / newFolderName);
                }
                std::string newMaterialName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Material", "Material name", newMaterialName)) {
                    runForFileType(m_currentPath / (newMaterialName + ".mat"),
                                   [&] {
                                       m_materialLoader->createNewMaterialResource(virtualPath / (newMaterialName + ".mat"));
                                   },
                                   [&] {
                                       m_serverRegistry.getSystem<ResourceSystem>().createMaterialResource(newMaterialName);
                                   });
                }

                std::string newSceneName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Scene", "Scene name", newSceneName)) {
                    std::string scenePath = m_currentPath.string() + newSceneName + ".tescene";
                    runForFileType(scenePath,
                                   [&] {
                                       m_sceneLoader->createScene(newSceneName, scenePath);
                                   },
                                   [&] {
                                       //m_serverRegistry.getSystem<ResourceSystem>().createScene(newSceneName, scenePath);
                                   });
                }
                /*
                std::string newScriptName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Script", "Script name", newScriptName)) {
                    TE_LOGGER_INFO("(WIP) New script name: {0}", newScriptName);
                }
               */
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
        return opened;
    }

    bool ContentBrowserPanel::openMenuPopupForFolderHierarchy(const std::filesystem::path& virtualPath) {
        bool opened = false;
        if (ImGui::BeginPopupContextItem()) {
            opened = true;
            if (ImGui::BeginMenu("New")) {
                std::string newFolderName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Folder", "New Folder", newFolderName)) {
                    m_fileSystem.createDirectory(virtualPath / newFolderName);
                }
                std::string newMaterialName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Material", "Material name", newMaterialName)) {
                    runForFileType(m_currentPath / (newMaterialName + ".mat"),
                                   [&] {
                                       m_materialLoader->createNewMaterialResource(virtualPath / (newMaterialName + ".mat"));
                                   },
                                   [&] {
                                       m_serverRegistry.getSystem<ResourceSystem>().createMaterialResource(newMaterialName);
                                   });
                }
                std::string newSceneName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Scene", "Scene name", newSceneName)) {
                    std::string scenePath = m_currentPath.string() + newSceneName + ".tescene";
                    runForFileType(scenePath,
                                   [&] {
                                       m_sceneLoader->createScene(newSceneName, scenePath);
                                   },
                                   [&] {
                                       //m_serverRegistry.getSystem<ResourceSystem>().createScene(newSceneName, scenePath);
                                   });
                }
                /*
                std::string newScriptName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Script", "Script name", newScriptName)) {
                    TE_LOGGER_INFO("(WIP) New script name: {0}", newScriptName);
                }
               */
                ImGui::EndMenu();
            }
            std::string buffer;
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "New name", buffer)) {
                renameFile(virtualPath, buffer);
            }
            if (ImGui::Button("Delete")) {
                deleteDirectory(virtualPath);
            }
            ImGui::EndPopup();
        }
        return opened;
    }

    void ContentBrowserPanel::runForFileType(const std::filesystem::path path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction) {
        FileType fileType = getFileType(path);
        if (fileType == FileType::Client) {
            clientFunction();
        } else if (fileType == FileType::Server) {
            serverFunction();
        } else if (fileType == FileType::Common) {
            clientFunction();
            serverFunction();
        }
    }

    void ContentBrowserPanel::validateCurrentPath() {
        std::filesystem::path path = m_currentPath;
        while (!path.empty() && !m_fileSystem.status(path).exists) {
            path = path.parent_path();
        }
        const std::string str = path.generic_string();
        const bool isValidRood = (str.find("editorAssetsClient://") == 0 || str.find("editorAssetsCommon://") == 0 || str.find("editorAssetsServer://") == 0);
        if (!isValidRood) {
            m_currentPath = "editorAssetsClient://";
        } else {
            m_currentPath = path;
        }
    }

    void ContentBrowserPanel::setupFileTypeRegistryHandlers() {
        m_fileTypeRegistry.registerHandler({".scene"}, {
                                               .extensions = {".scene"},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   // future scene loading
                                               }
                                           });
        m_fileTypeRegistry.registerHandler({m_sceneLoader->resourceExtension()}, {
                                               .extensions = {m_sceneLoader->resourceExtension()},
                                               .handler = [this](const std::filesystem::path& virtualPath) {
                                                   runForFileType(virtualPath,
                                                                  [&] {
                                                                      m_sceneLoader->loadScene(virtualPath);
                                                                  },
                                                                  [&] { /* server scene load */
                                                                  });
                                               }
                                           });

        m_fileTypeRegistry.registerHandler(m_modelLoader->sourceExtensions(), {
                                               .extensions = m_modelLoader->sourceExtensions(),
                                               .handler = [this](const std::filesystem::path& path) {
                                                   runForFileType(path,
                                                                  [&] { /* client mesh load */
                                                                  },
                                                                  [&] { /* server mesh load */
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Create Mesh")) {
                                                       runForFileType(virtualPath.string(),
                                                                      [&] {
                                                                          m_meshLoader->importMeshFile(virtualPath);
                                                                      },
                                                                      [&] {
                                                                          //m_serverRegistry.getSystem<ResourceSystem>().createMeshFromModelFile(path.string());
                                                                      });
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                                   if (ImGui::Button("Create Model")) {
                                                       runForFileType(virtualPath.string(),
                                                                      [&] {
                                                                          m_modelLoader->importModelFile(virtualPath);
                                                                      },
                                                                      [&] {
                                                                          // m_serverRegistry.getSystem<ResourceSystem>().createModelFromFile(path.string());
                                                                      });
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                                   if (ImGui::Button("Delete")) {
                                                       m_fileSystem.deleteFile(virtualPath);
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                               }
                                           });
        m_fileTypeRegistry.registerHandler({m_modelLoader->resourceExtension()}, {
                                               .extensions = {m_modelLoader->resourceExtension()},
                                               .handler = [this](const std::filesystem::path& virtualPath) {
                                                   runForFileType(virtualPath,
                                                                  [&] {
                                                                      ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();
                                                                      const std::string modelName = m_fileSystem.status(virtualPath).name.string();

                                                                      if (!resourceSystem.isModelRegistered(modelName)) {
                                                                          // The model's meshes/materials/textures live in a sibling folder named
                                                                          // after the model; register those first so the model's UUID references resolve.
                                                                          const std::filesystem::path dependencyFolder = m_fileSystem.getParentPath(virtualPath) / modelName;
                                                                          loadResourcesUnder(dependencyFolder);
                                                                          ensureResourceLoaded(virtualPath);
                                                                      }

                                                                      const std::shared_ptr<ModelResource> modelResource = resourceSystem.getModelResource(modelName);
                                                                      if (!modelResource) {
                                                                          TE_LOGGER_ERROR("[ContentBrowser] Could not load model '{0}' from '{1}'", modelName, virtualPath.string());
                                                                          return;
                                                                      }

                                                                      Scene& scene = m_clientRegistry.getSystem<SceneManager>().getActiveScene();
                                                                      resourceSystem.createEntityFromModelNode(scene, static_cast<Entity>(-1), modelResource->getUUID());
                                                                  },
                                                                  [&] {
                                                                      //Scene& scene = m_serverRegistry.getSystem<ScenesManager>().getActiveScene();
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Load into Scene")) {
                                                       runForFileType(virtualPath,
                                                                      [&] {
                                                                          ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();
                                                                          const std::string modelName = m_fileSystem.status(virtualPath).name.string();

                                                                          if (!resourceSystem.isModelRegistered(modelName)) {
                                                                              // The model's meshes/materials/textures live in a sibling folder named
                                                                              // after the model; register those first so the model's UUID references resolve.
                                                                              const std::filesystem::path dependencyFolder = m_fileSystem.getParentPath(virtualPath) / modelName;
                                                                              loadResourcesUnder(dependencyFolder);
                                                                              ensureResourceLoaded(virtualPath);
                                                                          }

                                                                          const std::shared_ptr<ModelResource> modelResource = resourceSystem.getModelResource(modelName);
                                                                          if (!modelResource) {
                                                                              TE_LOGGER_ERROR("[ContentBrowser] Could not load model '{0}' from '{1}'", modelName, virtualPath.string());
                                                                              return;
                                                                          }

                                                                          Scene& scene = m_clientRegistry.getSystem<SceneManager>().getActiveScene();
                                                                          resourceSystem.createEntityFromModelNode(scene, static_cast<Entity>(-1), modelResource->getUUID());
                                                                      },
                                                                      [&] {
                                                                      });
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                                   if (ImGui::Button("Delete")) {
                                                       // Resolve every dependency file (mesh/material/texture) via the UUID->path
                                                       // index BEFORE unregistering, since unregister removes the model from the
                                                       // cache. The runtime cascade runs synchronously on the main thread; the disk
                                                       // deletion is offloaded to a background thread.
                                                       std::vector<std::filesystem::path> pathsToDelete;
                                                       pathsToDelete.push_back(virtualPath);
                                                       collectModelDependencyPaths(virtualPath, pathsToDelete);

                                                       const std::string modelName = m_fileSystem.status(virtualPath).name.string();
                                                       if (m_clientRegistry.getSystem<ResourceSystem>().unregisterModelResource(modelName)) {
                                                           deletePhysicalPathsAsync(pathsToDelete);
                                                       }
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                               }

                                           });
        m_fileTypeRegistry.registerHandler({m_meshLoader->resourceExtension()}, {
                                               .extensions = {m_meshLoader->resourceExtension()},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   runForFileType(path,
                                                                  [&] {
                                                                      FileStatus status = m_fileSystem.status(path);
                                                                      ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();
                                                                      const std::string meshName = status.name.string();

                                                                      // Register the mesh and its sibling material/textures if they
                                                                      // came from another project and aren't registered yet.
                                                                      if (!resourceSystem.isMeshRegistered(meshName)) {
                                                                          loadResourcesUnder(m_fileSystem.getParentPath(path));
                                                                      }

                                                                      const std::shared_ptr<MeshResource> meshResource = resourceSystem.getMeshResource(meshName);
                                                                      const std::shared_ptr<MaterialResource> materialResource = resourceSystem.getMaterialResource(meshName);
                                                                      if (!meshResource || !materialResource) {
                                                                          TE_LOGGER_ERROR("[ContentBrowser] Could not load mesh '{0}' from '{1}'", meshName, path.string());
                                                                          return;
                                                                      }
                                                                      Scene& scene = m_clientRegistry.getSystem<SceneManager>().getActiveScene();
                                                                      Entity entity = scene.createEntity(status.name.string());
                                                                      scene.addComponent<MeshRenderer>(entity, MeshRenderer(meshResource->getUUID(), materialResource->getUUID()));
                                                                  },
                                                                  [&] { /* server mesh load */
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                               }
                                           });
        m_fileTypeRegistry.registerHandler({m_textureLoader->resourceExtension()}, {
                                               .extensions = {m_textureLoader->resourceExtension()},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   runForFileType(path,
                                                                  [&] { /* client texture load */
                                                                  },
                                                                  [&] { /* server texture load */
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Delete")) {
                                                       FileStatus fileStatus = m_fileSystem.status(virtualPath);
                                                       if (m_clientRegistry.getSystem<ResourceSystem>().unregisterTextureResource(fileStatus.name.string())) {
                                                           m_fileSystem.deleteFile(virtualPath);
                                                       }
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                               }
                                           });
        m_fileTypeRegistry.registerHandler({m_textureLoader->sourceExtensions().front()}, {
                                               .extensions = {m_textureLoader->sourceExtensions().front()},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   runForFileType(path,
                                                                  [&] { /* client texture load */
                                                                  },
                                                                  [&] { /* server texture load */
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Create Texture")) {
                                                       runForFileType(virtualPath.string(),
                                                                      [&] {
                                                                          m_textureLoader->importTextureFile(virtualPath);
                                                                      },
                                                                      [&] {
                                                                          //m_serverRegistry.getSystem<ResourceSystem>().createTextureFromFile(path.string());
                                                                      });
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                                   if (ImGui::Button("Delete")) {
                                                       m_fileSystem.deleteFile(virtualPath);
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                               }
                                           });
        m_fileTypeRegistry.registerHandler({m_materialLoader->resourceExtension()}, {
                                               .extensions = {m_materialLoader->resourceExtension()},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   runForFileType(path,
                                                                  [&] { /* client material load */
                                                                  },
                                                                  [&] { /* server material load */
                                                                  });
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Delete")) {
                                                       FileStatus fileStatus = m_fileSystem.status(virtualPath);
                                                       if (m_clientRegistry.getSystem<ResourceSystem>().unregisterMaterialResource(fileStatus.name.string())) {
                                                           m_fileSystem.deleteFile(virtualPath);
                                                       }
                                                       ImGui::CloseCurrentPopup();
                                                   }
                                               }
                                           });

        m_fileTypeRegistry.registerHandler({".cpp"}, {
                                               .extensions = {".cpp"},
                                               .handler = [this](const std::filesystem::path& path) {
                                                   // Handle .cpp files
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   // Context menu for .cpp files
                                               },
                                               .iconPath = "editorAssets://icons/cpp.png"
                                           });

        m_fileTypeRegistry.registerHandler({"*"}, {
                                               .extensions = {"*"},
                                               .handler = [this](const std::filesystem::path& path) {
                                               },
                                               .onContextMenu = [this](const std::filesystem::path& virtualPath) {
                                                   if (ImGui::Button("Delete")) {
                                                       if (m_fileSystem.isDirectory(virtualPath)) {
                                                           deleteDirectory(virtualPath);
                                                           ImGui::CloseCurrentPopup();
                                                       } else {
                                                           deleteFile(virtualPath);
                                                           ImGui::CloseCurrentPopup();
                                                       }
                                                   }
                                               }
                                           });
    }

    void ContentBrowserPanel::rebuildCache() {
        m_rootNode = buildFileNode("editorAssetsClient://");
        m_currentNode = findNode(m_rootNode, m_currentPath);
        m_dirtyCache = false;
    }

    void ContentBrowserPanel::renderSettingsBar() {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);

        ImGui::SetNextItemWidth(200.f);
        ImGui::SliderFloat("Icon Size##cb", &m_iconSize, 24.f, 128.f, "%.0f px");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.f);
        ImGui::SliderFloat("Padding##cb", &m_iconPadding, 4.f, 32.f, "%.0f px");
        ImGui::SameLine();
        if (ImGui::Button("Small##cb")) {
            m_iconSize = 32.f;
            m_iconPadding = 8.f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Medium##cb")) {
            m_iconSize = 64.f;
            m_iconPadding = 16.f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Large##cb")) {
            m_iconSize = 96.f;
            m_iconPadding = 24.f;
        }

        ImGui::PopStyleVar();
        ImGui::Separator();
    }

    std::vector<std::string> ContentBrowserPanel::computeWrappedLines(
        const std::string& label, float maxWidth, int maxLines) const {
        std::vector<std::string> lines;
        std::string remaining = label;

        while (!remaining.empty() && static_cast<int>(lines.size()) < maxLines) {
            int fitLen = static_cast<int>(remaining.size());
            while (fitLen > 0 && ImGui::CalcTextSize(remaining.substr(0, fitLen).c_str()).x > maxWidth)
                --fitLen;

            if (fitLen == static_cast<int>(remaining.size())) {
                lines.push_back(remaining);
                remaining.clear();
            } else if (static_cast<int>(lines.size()) == maxLines - 1) {
                std::string truncated = remaining.substr(0, fitLen);
                while (!truncated.empty() && ImGui::CalcTextSize((truncated + "...").c_str()).x > maxWidth)
                    truncated.pop_back();
                lines.push_back(truncated + "...");
                remaining.clear();
            } else {
                int breakPos = fitLen;
                while (breakPos > 0 && remaining[breakPos] != ' ' && remaining[breakPos] != '_' && remaining[breakPos] != '.')
                    --breakPos;
                if (breakPos == 0) breakPos = fitLen;
                lines.push_back(remaining.substr(0, breakPos));
                remaining = remaining.substr(breakPos);
                if (!remaining.empty() && remaining[0] == ' ')
                    remaining = remaining.substr(1);
            }
        }
        return lines;
    }


    IconEntry ContentBrowserPanel::resolveDisplayTexture(const FileNode& child) {
        if (child.isDirectory) {
            return child.icon;
        }

        ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();
        const std::string extension = child.virtualPath.extension().string();
        const std::string textureExtension = m_textureLoader->resourceExtension();

        if (extension == textureExtension) {
            const std::string name = m_fileSystem.status(child.virtualPath).name.string();
            if (resourceSystem.isTextureRegistered(name)) {
                if (auto textureResource = resourceSystem.getTextureResource(name))
                    return m_iconRegistry.getThumbnail(textureResource->getUUID(), m_fileTypeRegistry.getHandler(child.virtualPath).iconPath);
            }
            return child.icon;
        }

        const auto& sourceExtensions = m_textureLoader->sourceExtensions();
        const bool isImageSource = std::ranges::any_of(sourceExtensions, [&](const std::string& e) {
            return e == extension;
        });
        if (isImageSource) {
            return m_iconRegistry.getIcon(child.virtualPath);
        }

        return child.icon;
    }


    bool ContentBrowserPanel::renderFileItem(const FileNode& child, int childIndex,
                                             const CellLayout& layout) {
        const float lineH = ImGui::GetTextLineHeightWithSpacing();
        const float totalCellH = layout.iconSize
                                 + ImGui::GetStyle().ItemSpacing.y
                                 + layout.maxTextLines * lineH;
        const auto lines = computeWrappedLines(child.label, layout.textMaxWidth, layout.maxTextLines);

        const float colW = ImGui::GetContentRegionAvail().x;
        const ImVec2 cellStart = ImGui::GetCursorScreenPos();

        if (child.isDirectory) {
            m_folderDropZones.push_back({
                cellStart,
                {cellStart.x + colW, cellStart.y + totalCellH},
                child.virtualPath
            });
        }

        const float centreOffset = (colW - layout.iconSize) * 0.5f;
        const ImVec2 iconMin = {
            ImGui::GetCursorScreenPos().x + centreOffset,
            ImGui::GetCursorScreenPos().y
        };
        const ImVec2 iconMax = {iconMin.x + layout.iconSize, iconMin.y + layout.iconSize};

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centreOffset);
        ImGui::InvisibleButton("##icon", {layout.iconSize, layout.iconSize});

        if (isSelected(child.virtualPath) || ImGui::IsItemHovered()) {
            const float verticalPadding = layout.cellPadding * 0.25f;
            ImGui::GetWindowDrawList()->AddRectFilled(
                {cellStart.x, cellStart.y - verticalPadding},
                {cellStart.x + colW, cellStart.y + totalCellH},
                IM_COL32(80, 80, 80, 180), 6.f);
        }

        IconEntry icon = resolveDisplayTexture(child);
        ImVec2 iconSizeMin;
        ImVec2 iconSizeMax;
        if (icon.size.x > 0 && icon.size.y > 0) {
            const float cellW = iconMax.x - iconMin.x;
            const float cellH = iconMax.y - iconMin.y;
            const float scaleW = cellW / static_cast<float>(icon.size.x);
            const float scaleH = cellH / static_cast<float>(icon.size.y);
            const float scale = std::min(scaleW, scaleH);

            const float drawW = icon.size.x * scale;
            const float drawH = icon.size.y * scale;
            const float offX = (cellW - drawW) * 0.5f;
            const float offY = (cellH - drawH) * 0.5f;

            iconSizeMin = {iconMin.x + offX, iconMin.y + offY};
            iconSizeMax = {iconMin.x + offX + drawW, iconMin.y + offY + drawH};
        } else {
            iconSizeMin = iconMin;
            iconSizeMax = iconMax;
        }

        if (m_isDragging) {
            const ImVec2 cellEnd = {
                cellStart.x + ImGui::GetContentRegionAvail().x,
                cellStart.y + layout.iconSize
                + layout.maxTextLines * ImGui::GetTextLineHeightWithSpacing()
            };

            const ImVec2 rMin = {
                std::min(m_dragStart.x, m_dragEnd.x),
                std::min(m_dragStart.y, m_dragEnd.y)
            };
            const ImVec2 rMax = {
                std::max(m_dragStart.x, m_dragEnd.x),
                std::max(m_dragStart.y, m_dragEnd.y)
            };

            const bool intersects = rMin.x < cellEnd.x && rMax.x > cellStart.x &&
                                    rMin.y < cellEnd.y && rMax.y > cellStart.y;

            if (intersects)
                m_selectedPaths.insert(child.virtualPath);
            else if (!ImGui::GetIO().KeyCtrl)
                m_selectedPaths.erase(child.virtualPath);
        }

        ImGui::GetWindowDrawList()->AddImage(icon.id, iconSizeMin, iconSizeMax);

        if (ImGui::IsItemHovered() && !lines.empty() && lines.back().ends_with("...")) {
            ImGui::SetTooltip("%s", child.label.c_str());
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            handleItemClick(child.virtualPath, childIndex);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            openFolderOrFile(child.virtualPath);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (!isSelected(child.virtualPath)) {
                m_selectedPaths.clear();
                m_selectedPaths.insert(child.virtualPath);
                m_anchorPath = child.virtualPath;
            }
        }

        const bool menuOpened = openMenuPopupItem(child.virtualPath);

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM",
                                      child.label.c_str(), child.label.size() + 1);
            ImGui::Text("%s", child.label.c_str());
            ImGui::EndDragDropSource();
        }

        // Label lines — centred, fixed to maxTextLines rows
        for (const std::string& line: lines) {
            const float lineW = ImGui::CalcTextSize(line.c_str()).x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colW - lineW) * 0.5f);
            ImGui::TextUnformatted(line.c_str());
        }

        // Pad remaining lines so every cell has identical height
        const int unusedLines = layout.maxTextLines - static_cast<int>(lines.size());
        if (unusedLines > 0)
            ImGui::Dummy({0.f, unusedLines * lineH});


        return menuOpened;
    }

    ContentBrowserPanel::FileNode ContentBrowserPanel::buildFileNode(const std::filesystem::path& virtualPath) {
        FileNode node;
        node.virtualPath = virtualPath;
        FileStatus status = m_fileSystem.status(virtualPath);
        node.label = status.name.string() + status.extension.string();
        node.isDirectory = status.isDirectory;
        node.icon = m_iconRegistry.getIcon("editorAssets://icons/folder.png");

        for (const auto& child: m_fileSystem.list(virtualPath, false)) {
            FileStatus cs = m_fileSystem.status(child);
            FileNode childNode;
            childNode.virtualPath = child;
            childNode.label = cs.name.string() + cs.extension.string();
            childNode.isDirectory = cs.isDirectory;
            if (cs.isDirectory) {
                node.hasSubDirs = true;
                childNode = buildFileNode(child);
                childNode.icon = m_iconRegistry.getIcon("editorAssets://icons/folder.png");
            } else {
                const std::filesystem::path& iconPath = m_fileTypeRegistry.getHandler(child).iconPath;
                childNode.icon = m_iconRegistry.getIcon(iconPath);
                if (childNode.icon.id == nullptr) {
                    childNode.icon = m_iconRegistry.getIcon("editorAssets://icons/default.png");
                }
            }
            node.children.push_back(std::move(childNode));
        }
        return node;
    }

    const ContentBrowserPanel::FileNode* ContentBrowserPanel::findNode(const FileNode& node, std::filesystem::path virtualPath) {
        if (node.virtualPath == virtualPath) {
            return &node;
        }

        for (const FileNode& child: node.children) {
            if (const FileNode* found = findNode(child, virtualPath)) {
                return found;
            }
        }
        return nullptr;
    }

    void ContentBrowserPanel::unregisterNodeResources(const FileNode& node) {
        if (node.isDirectory) {
            for (const FileNode& child: node.children) {
                unregisterNodeResources(child);
            }
            return;
        }
        unregisterResourceForFile(node.virtualPath);
    }

    void ContentBrowserPanel::unregisterResourceForFile(const std::filesystem::path& virtualPath) {
        const std::string name = m_fileSystem.status(virtualPath).name.string();
        const std::string extension = virtualPath.extension().string();

        ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();

        if (extension == m_modelLoader->resourceExtension()) {
            resourceSystem.unregisterModelResource(name);
        } else if (extension == m_materialLoader->resourceExtension()) {
            resourceSystem.unregisterMaterialResource(name);
        } else if (extension == m_textureLoader->resourceExtension()) {
            resourceSystem.unregisterTextureResource(name);
        } else if (extension == m_meshLoader->resourceExtension()) {
            resourceSystem.unregisterMeshResource(name);
        } else if (extension == m_sceneLoader->resourceExtension()) {
            resourceSystem.unregisterSceneResource(name);
        }
    }

    bool ContentBrowserPanel::isResourceRegisteredForFile(const std::filesystem::path& virtualPath) {
        const std::string name = m_fileSystem.status(virtualPath).name.string();
        const std::string extension = virtualPath.extension().string();

        ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();

        if (extension == m_modelLoader->resourceExtension()) {
            return resourceSystem.isModelRegistered(name);
        }
        if (extension == m_materialLoader->resourceExtension()) {
            return resourceSystem.isMaterialRegistered(name);
        }
        if (extension == m_textureLoader->resourceExtension()) {
            return resourceSystem.isTextureRegistered(name);
        }
        if (extension == m_meshLoader->resourceExtension()) {
            return resourceSystem.isMeshRegistered(name);
        }
        if (extension == m_sceneLoader->resourceExtension()) {
            return resourceSystem.isSceneRegistered(name);
        }
        return false;
    }

    void ContentBrowserPanel::handleFileWatcherEvent(const FileChangedEvent& event) {
        // Runs on the main thread (FileWatcher::poll() is driven from the editor loop), so both the
        // ResourceSystem and the index can be touched safely here.
        const std::filesystem::path& virtualPath = event.virtualPath;
        if (!isResourceExtension(virtualPath.extension().string())) {
            return;
        }

        switch (event.action) {
            case FileChangeAction::Added:
            case FileChangeAction::Modified: {
                UUID uuid;
                if (ResourcePathIndex::readHeaderUUID(m_fileSystem, virtualPath, uuid)) {
                    m_pathIndex.set(uuid, virtualPath);
                }
                break;
            }
            case FileChangeAction::Removed: {
                // Handles external (Explorer) deletions and acts as a safety net for the per-file
                // Removed events emitted by recursive removals. In-editor deletes already
                // unregistered synchronously, so the guard makes this a no-op for them.
                if (isResourceRegisteredForFile(virtualPath)) {
                    unregisterResourceForFile(virtualPath);
                }
                m_pathIndex.removeByPath(virtualPath);
                break;
            }
            case FileChangeAction::Renamed: {
                if (!event.renamedFromVirtualPath.empty()) {
                    m_pathIndex.removeByPath(event.renamedFromVirtualPath);
                }
                UUID uuid;
                if (ResourcePathIndex::readHeaderUUID(m_fileSystem, virtualPath, uuid)) {
                    m_pathIndex.set(uuid, virtualPath);
                }
                break;
            }
        }
    }

    void ContentBrowserPanel::collectModelDependencyPaths(const std::filesystem::path& modelVirtualPath,
                                                          std::vector<std::filesystem::path>& outPaths) {
        ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();
        const std::string modelName = m_fileSystem.status(modelVirtualPath).name.string();
        const std::shared_ptr<ModelResource> modelResource = resourceSystem.getModelResource(modelName);
        if (!modelResource) {
            return; // Unloaded model: only the .temodel file itself is removed.
        }

        const auto addPath = [&](const UUID& uuid) {
            if (uuid.isNull()) {
                return;
            }
            const std::filesystem::path path = m_pathIndex.getPath(uuid);
            if (!path.empty()) {
                outPaths.push_back(path);
            }
        };

        for (const UUID& meshUUID: modelResource->getMeshesUUIDs()) {
            addPath(meshUUID);
        }
        for (const UUID& materialUUID: modelResource->getMaterialUUIDs()) {
            addPath(materialUUID);
            const std::shared_ptr<MaterialResource> material = resourceSystem.getMaterialResource(materialUUID);
            if (material) {
                addPath(material->getAlbedoMapUUID());
                addPath(material->getNormalMapUUID());
                addPath(material->getMetallicMapUUID());
                addPath(material->getRoughnessMapUUID());
                addPath(material->getAmbientOcclusionMapUUID());
                addPath(material->getEmissionMapUUID());
            }
        }
    }

    void ContentBrowserPanel::deletePhysicalPathsAsync(std::vector<std::filesystem::path> virtualPaths) {
        // Resolve virtual -> physical on the main thread (mount table access), then delete the files
        // in parallel on a detached worker so the UI never blocks. The FileWatcher refreshes the
        // view (and the index) as the files disappear.
        std::vector<std::filesystem::path> physicalPaths;
        physicalPaths.reserve(virtualPaths.size());
        for (const std::filesystem::path& virtualPath: virtualPaths) {
            const std::filesystem::path physical = m_fileSystem.resolve(virtualPath);
            if (!physical.empty()) {
                physicalPaths.push_back(physical);
            }
        }
        if (physicalPaths.empty()) {
            return;
        }

        std::thread([physicalPaths = std::move(physicalPaths)]() {
            parallelFor(physicalPaths.size(), [&](size_t i) {
                std::error_code ec;
                std::filesystem::remove_all(physicalPaths[i], ec);
                if (ec) {
                    TE_LOGGER_ERROR("[ContentBrowser] Failed to delete '{0}': {1}", physicalPaths[i].string(), ec.message());
                }
            });
        }).detach();
    }

    bool ContentBrowserPanel::ensureResourceLoaded(const std::filesystem::path& virtualPath) {
        const FileStatus status = m_fileSystem.status(virtualPath);
        if (!status.exists || status.isDirectory) {
            return false;
        }

        const std::string extension = status.extension.string();
        const std::string name = status.name.string();
        ResourceSystem& resourceSystem = m_clientRegistry.getSystem<ResourceSystem>();

        // Deserialize (which registers) only when the resource is not already present.
        // register*Resource handles UUID collisions with resources from other projects.
        const auto load = [&](const std::function<void(Buffer&)>& deserialize) {
            Buffer buffer;
            if (!m_fileSystem.read(virtualPath, buffer)) {
                TE_LOGGER_ERROR("[ContentBrowser] Failed to read resource file: {0}", virtualPath.string());
                return;
            }
            deserialize(buffer);
        };

        if (extension == m_textureLoader->resourceExtension()) {
            if (resourceSystem.isTextureRegistered(name)) return true;
            load([this](Buffer& b) {
                m_textureLoader->deserializeTextureResource(b);
            });
        } else if (extension == m_materialLoader->resourceExtension()) {
            if (resourceSystem.isMaterialRegistered(name)) return true;
            load([this](Buffer& b) {
                m_materialLoader->deserializeMaterialResource(b);
            });
        } else if (extension == m_meshLoader->resourceExtension()) {
            if (resourceSystem.isMeshRegistered(name)) return true;
            load([this](Buffer& b) {
                m_meshLoader->deserializeMeshResource(b);
            });
        } else if (extension == m_modelLoader->resourceExtension()) {
            if (resourceSystem.isModelRegistered(name)) return true;
            load([this](Buffer& b) {
                m_modelLoader->deserializeModelResource(b);
            });
        } else {
            return false;
        }
        return true;
    }

    void ContentBrowserPanel::loadResourcesUnder(const std::filesystem::path& virtualFolder) {
        if (!m_fileSystem.status(virtualFolder).exists) {
            return;
        }

        std::vector<std::filesystem::path> files = m_fileSystem.list(virtualFolder, true);

        const auto priority = [&](const std::filesystem::path& file) {
            const std::string extension = m_fileSystem.status(file).extension.string();
            if (extension == m_textureLoader->resourceExtension()) return 0;
            if (extension == m_materialLoader->resourceExtension()) return 1;
            if (extension == m_meshLoader->resourceExtension()) return 2;
            if (extension == m_modelLoader->resourceExtension()) return 3;
            return 1000;
        };

        std::ranges::sort(files, [&](const std::filesystem::path& a, const std::filesystem::path& b) {
            return priority(a) < priority(b);
        });

        for (const std::filesystem::path& file: files) {
            ensureResourceLoaded(file);
        }
    }


    FileType ContentBrowserPanel::getFileType(const std::filesystem::path& virtualPath) {
        //HACK: Check alias here. This is not the best solution but will do it for now
        std::string alias = virtualPath.generic_string().substr(0, virtualPath.generic_string().find_first_of("://") + 3);
        if (alias == "editorAssetsClient://") {
            return FileType::Client;
        } else if (alias == "editorAssetsServer://") {
            return FileType::Server;
        } else if (alias == "editorAssetsCommon://") {
            return FileType::Common;
        } else {
            TE_LOGGER_CRITICAL("Unknown file type: \n\tRoot folder: {0}\n\tPath: {1}", alias, virtualPath.string());
            return FileType::Common;
        }
    }

    bool ContentBrowserPanel::isSelected(const std::filesystem::path& virtualPath) const {
        return m_selectedPaths.contains(virtualPath);
    }

    void ContentBrowserPanel::handleItemClick(const std::filesystem::path& virtualPath, int childIndex) {
        const ImGuiIO& io = ImGui::GetIO();
        const bool ctrl = io.KeyCtrl;
        const bool shift = io.KeyShift;

        if (shift && !m_anchorPath.empty()) {
            int anchorIndex = -1;
            int i = 0;
            for (const FileNode& child: m_currentNode->children) {
                if (child.virtualPath == m_anchorPath) {
                    anchorIndex = i;
                    break;
                }
                ++i;
            }

            if (anchorIndex != -1) {
                rangeSelect(anchorIndex, childIndex, ctrl);
            } else {
                m_selectedPaths.clear();
                m_selectedPaths.insert(virtualPath);
                m_anchorPath = virtualPath;
            }
            return;
        }

        if (ctrl) {
            if (m_selectedPaths.contains(virtualPath)) {
                m_selectedPaths.erase(virtualPath);
            } else {
                m_selectedPaths.insert(virtualPath);
            }
            m_anchorPath = virtualPath;
            return;
        }

        m_selectedPaths.clear();
        m_selectedPaths.insert(virtualPath);
        m_anchorPath = virtualPath;
    }

    void ContentBrowserPanel::rangeSelect(int from, int to, bool additive) {
        if (!additive) {
            m_selectedPaths.clear();
        }

        // Normalize so we always iterate low → high.
        const int lo = std::min(from, to);
        const int hi = std::max(from, to);

        int i = 0;
        for (const FileNode& child: m_currentNode->children) {
            if (i > hi) {
                break;
            }
            if (i >= lo) {
                m_selectedPaths.insert(child.virtualPath);
            }
            ++i;
        }
    }

    ContentBrowserPanel::CellLayout ContentBrowserPanel::makeCellLayout(float iconSize, float iconPadding) {
        CellLayout l;
        l.iconSize = iconSize;
        l.cellPadding = iconPadding;
        l.columnWidth = iconSize + iconPadding * 2.0f;
        l.textMaxWidth = l.columnWidth - 4.0f;
        l.maxTextLines = 3;
        return l;
    }

    void ContentBrowserPanel::onDropCallback(GLFWwindow* window, int count, const char** paths) {
        if (!s_dropTarget) {
            return;
        }

        // GLFW does not report the drop position, so query the cursor ourselves and
        // convert it to the same virtual-desktop space that ImGui uses with viewports.
        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(window, &cursorX, &cursorY);
        int winX = 0;
        int winY = 0;
        glfwGetWindowPos(window, &winX, &winY);

        s_dropTarget->m_externalDropPos = {
            static_cast<float>(winX) + static_cast<float>(cursorX),
            static_cast<float>(winY) + static_cast<float>(cursorY)
        };

        s_dropTarget->m_pendingExternalDrops.clear();
        for (int i = 0; i < count; ++i) {
            s_dropTarget->m_pendingExternalDrops.emplace_back(paths[i]);
        }
        s_dropTarget->m_hasExternalDrop = true;
    }

    void ContentBrowserPanel::processExternalDrop() {
        if (!m_hasExternalDrop) {
            return;
        }
        m_hasExternalDrop = false;

        if (m_pendingExternalDrops.empty()) {
            return;
        }

        const ImVec2 pos = m_externalDropPos;

        // Ignore drops that landed outside this panel's window.
        const bool insidePanel = pos.x >= m_panelMin.x && pos.x <= m_panelMax.x &&
                                 pos.y >= m_panelMin.y && pos.y <= m_panelMax.y;
        if (!insidePanel) {
            m_pendingExternalDrops.clear();
            return;
        }

        // Prefer a specific folder under the cursor (grid item or hierarchy node),
        // otherwise fall back to the folder currently being viewed.
        std::filesystem::path targetDir = m_currentPath;
        for (const DropZone& zone: m_folderDropZones) {
            if (pos.x >= zone.min.x && pos.x <= zone.max.x &&
                pos.y >= zone.min.y && pos.y <= zone.max.y) {
                targetDir = zone.virtualPath;
                break;
            }
        }

        importExternalPaths(m_pendingExternalDrops, targetDir);
        m_pendingExternalDrops.clear();
    }

    std::filesystem::path ContentBrowserPanel::makeUniquePhysicalPath(const std::filesystem::path& desired) const {
        std::error_code ec;
        if (!std::filesystem::exists(desired, ec)) {
            return desired;
        }

        const std::filesystem::path parent = desired.parent_path();
        const std::string stem = desired.stem().string();
        const std::string extension = desired.extension().string();

        for (int i = 1; i < 10000; ++i) {
            std::filesystem::path candidate = parent / (stem + " (" + std::to_string(i) + ")" + extension);
            if (!std::filesystem::exists(candidate, ec)) {
                return candidate;
            }
        }
        return desired;
    }

    void ContentBrowserPanel::importExternalPaths(const std::vector<std::filesystem::path>& osPaths,
                                                  const std::filesystem::path& targetVirtualDir) {
        // Resolve every source/destination pair up front on the main thread, since it
        // touches the mount table and the (main-thread) node layout. The actual byte
        // copy is deferred to a worker thread so the UI never blocks — the FileWatcher
        // refreshes the view as the copied files appear.
        struct CopyJob {
            std::filesystem::path source;
            std::filesystem::path destination;
            bool isDirectory = false;
        };

        std::vector<CopyJob> jobs;
        jobs.reserve(osPaths.size());

        for (const std::filesystem::path& osPath: osPaths) {
            std::error_code ec;
            if (!std::filesystem::exists(osPath, ec)) {
                TE_LOGGER_WARN("[ContentBrowser] Dropped path does not exist: '{0}'", osPath.string());
                continue;
            }

            const std::filesystem::path destVirtual = targetVirtualDir / osPath.filename();
            std::filesystem::path destPhysical = m_fileSystem.resolve(destVirtual);
            if (destPhysical.empty()) {
                TE_LOGGER_ERROR("[ContentBrowser] Could not resolve drop destination for '{0}'", destVirtual.string());
                continue;
            }

            jobs.push_back({
                osPath,
                makeUniquePhysicalPath(destPhysical),
                std::filesystem::is_directory(osPath, ec)
            });
        }

        if (jobs.empty()) {
            return;
        }

        std::thread([jobs = std::move(jobs)]() {
            for (const CopyJob& job: jobs) {
                std::error_code ec;
                std::filesystem::create_directories(job.destination.parent_path(), ec);

                if (job.isDirectory) {
                    std::filesystem::copy(job.source, job.destination,
                                          std::filesystem::copy_options::recursive, ec);
                } else {
                    std::filesystem::copy_file(job.source, job.destination, ec);
                }

                if (ec) {
                    TE_LOGGER_ERROR("[ContentBrowser] Failed to import '{0}' -> '{1}': {2}",
                                    job.source.string(), job.destination.string(), ec.message());
                } else {
                    TE_LOGGER_INFO("[ContentBrowser] Imported '{0}' -> '{1}'",
                                   job.source.string(), job.destination.string());
                }
            }
        }).detach();
    }
}
