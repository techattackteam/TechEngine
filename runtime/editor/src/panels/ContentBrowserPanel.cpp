#include "PanelsManager.hpp"
#include "project/ProjectManager.hpp"
#include "ContentBrowserPanel.hpp"

#include "core/Logger.hpp"
#include "resources/ResourcesManager.hpp"
#include "resources/mesh/AssimpLoader.hpp"
#include "resources/mesh/MeshManager.hpp"
//#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    ContentBrowserPanel::ContentBrowserPanel(SystemsRegistry& editorRegistry,
                                             SystemsRegistry& clientRegistry,
                                             SystemsRegistry& serverRegistry,
                                             PanelsManager& panelsManager) : m_editorRegistry(editorRegistry),
                                                                             m_clientRegistry(clientRegistry),
                                                                             m_serverRegistry(serverRegistry),
                                                                             m_panelsManager(panelsManager) {
    }


    void ContentBrowserPanel::onInit() {
        m_currentPath = m_editorRegistry.getSystem<ProjectManager>().getAssetsPath();
    }

    void ContentBrowserPanel::onUpdate() {
        if (ImGui::BeginTable("Content", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Outline", 0, 250.f);
            ImGui::TableSetupColumn("Files", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::BeginChild("Folders");

            ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
            renderDirectoryHierarchy(m_editorRegistry.getSystem<ProjectManager>().getAssetsPath().string() + "\\client");
            renderDirectoryHierarchy(m_editorRegistry.getSystem<ProjectManager>().getAssetsPath().string() + "\\common");
            renderDirectoryHierarchy(m_editorRegistry.getSystem<ProjectManager>().getAssetsPath().string() + "\\server");
            ImGui::PopStyleVar();

            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);

            ImGui::BeginChild("Files", {ImGui::GetContentRegionAvail().x, ImGui::GetWindowSize().y});
            renderFiles(m_currentPath);
            ImGui::EndChild();
            ImGui::EndTable();
        }
    }

    void ContentBrowserPanel::renderDirectoryHierarchy(const std::filesystem::path& directoryPath) {
        ImGuiTreeNodeFlags flags = ((m_currentPath == directoryPath) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanFullWidth |
                                   ImGuiTreeNodeFlags_OpenOnArrow;
        std::string relativePath = directoryPath.string().substr(directoryPath.string().find_last_of('\\') + 1);
        bool open = ImGui::TreeNodeEx(relativePath.c_str(), flags);
        if (ImGui::IsItemClicked()) {
            m_currentPath = directoryPath;
            m_selectedPath = directoryPath;
        }
        if (open) {
            renderDirectoryHierarchyRecurse(directoryPath); // Recursive call to render subdirectories
            ImGui::TreePop();
        }
    }

    void ContentBrowserPanel::renderDirectoryHierarchyRecurse(const std::filesystem::path& pathToRecurse) {
        for (auto& directoryEntry: std::filesystem::directory_iterator(pathToRecurse)) {
            if (!directoryEntry.is_directory()) {
                continue;
            }
            const std::filesystem::path& path = directoryEntry.path();
            auto relativePath = std::filesystem::relative(path, m_editorRegistry.getSystem<ProjectManager>().getProjectPath());
            std::string filenameString = relativePath.filename().string();
            bool hasSubDirs = false;

            for (auto& subDir: std::filesystem::directory_iterator(path)) {
                if (subDir.is_directory()) {
                    hasSubDirs = true;
                    break;
                }
            }

            // Define flags based on whether the node is selected or not
            ImGuiTreeNodeFlags flags = ((m_currentPath == path) ? ImGuiTreeNodeFlags_Selected : 0) |
                                       ImGuiTreeNodeFlags_OpenOnArrow |
                                       (hasSubDirs ? 0 : ImGuiTreeNodeFlags_Leaf) |
                                       ImGuiTreeNodeFlags_SpanFullWidth;


            bool open = ImGui::TreeNodeEx(filenameString.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_currentPath = path;
                m_selectedPath = path;
            }
            if (open) {
                renderDirectoryHierarchyRecurse(path); // Recursive call to render subdirectories
                ImGui::TreePop();
            }
        }
    }

    void ContentBrowserPanel::renderFiles(const std::filesystem::path& directoryPath) {
        bool opened = false;
        for (auto& directoryEntry: std::filesystem::directory_iterator(directoryPath)) {
            const auto& path = directoryEntry.path();
            auto relativePath = std::filesystem::relative(path, m_editorRegistry.getSystem<ProjectManager>().getAssetsPath());
            std::string filenameString = relativePath.filename().string();
            if (m_selectedPath == path.string()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
            ImGui::Button(filenameString.c_str());
            if (m_selectedPath == path.string()) {
                ImGui::PopStyleColor(2);
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", filenameString.c_str(), filenameString.size() + 1);
                ImGui::Text("%s", filenameString.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    openFolderOrFile(path.string());
                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    m_selectedPath = path.string();
                }
            }
            if (openMenuPopupItem(path)) {
                opened = true;
            }
        }
        if (!opened) {
            if (openMenuPopupWindow(m_currentPath)) {
                opened = true;
            }
        }
    }

    void ContentBrowserPanel::openFolderOrFile(const std::string& path) {
        if (std::filesystem::is_directory(path)) {
            m_currentPath = path;
        } else {
            std::string filename = path.substr(path.find_last_of('\\') + 1);
            std::string extension = filename.substr(filename.find_last_of('.'));
            if (extension == ".scene") {
                std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
                /*runFunctionBasedOnFileType(path,
                                           [&] {
                                               m_client.systemsRegistry.getSystem<SceneManager>().loadScene(filenameWithoutExtension);
                                           },
                                           [&] {
                                               m_server.systemsRegistry.getSystem<SceneManager>().loadScene(filenameWithoutExtension);
                                           }
                );*/
            } else if (extension == ".mat") {
                std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
                //m_panelsManager.openMaterialEditor(filenameWithoutExtension, path);
            } else if (extension == ".fbx" || extension == ".obj") {
                runFunctionBasedOnFileType(path,
                                           [&] {
                                               m_clientRegistry.getSystem<ResourcesManager>().loadModelFile(path);
                                           },
                                           [&] {
                                               m_serverRegistry.getSystem<ResourcesManager>().loadModelFile(path);
                                           });
            } else if (extension == ".TE_mesh") {
                runFunctionBasedOnFileType(path,
                                           [&] {
                                               m_clientRegistry.getSystem<ResourcesManager>().registerStaticMesh(path);
                                           },
                                           [&] {
                                               m_serverRegistry.getSystem<ResourcesManager>().registerStaticMesh(path);
                                           });
            }
        }
    }

    void ContentBrowserPanel::renameFile(const std::string& filename, const std::string& newName) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        std::filesystem::rename(m_currentPath / filename, m_currentPath / (newName + extension));
    }

    void ContentBrowserPanel::tryLoadModel(const std::string& filepath) {
        //assimpLoader.loadModel(filepath);
    }

    void ContentBrowserPanel::deleteFile(const std::string& filename) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        if (extension == ".scene") {
            std::string path = m_currentPath.string() + "\\" + filename;

            /*if (m_client.systemsRegistry.getSystem<SceneManager>().hasScene(path)) {
                m_client.systemsRegistry.getSystem<SceneManager>().deleteScene(path);
                std::filesystem::remove(path);
            } else if (m_server.systemsRegistry.getSystem<SceneManager>().hasScene(path)) {
                m_server.systemsRegistry.getSystem<SceneManager>().deleteScene(path);
                std::filesystem::remove(path);
            }*/
        } else if (extension == ".mat") {
            std::string path = m_currentPath.string() + "\\" + filename;
            std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
            /*if (m_client.systemsRegistry.getSystem<MaterialManager>().materialExists(filenameWithoutExtension)) {
                m_client.systemsRegistry.getSystem<MaterialManager>().deleteMaterial(filenameWithoutExtension);
                std::filesystem::remove(path);
            } else if (m_server.systemsRegistry.getSystem<MaterialManager>().materialExists(filenameWithoutExtension)) {
                m_server.systemsRegistry.getSystem<MaterialManager>().deleteMaterial(filenameWithoutExtension);
                std::filesystem::remove(path);
            }*/
        }
    }

    bool ContentBrowserPanel::openMenuPopupItem(const std::filesystem::path& path) {
        bool opened = false;
        if (ImGui::BeginPopupContextItem()) {
            opened = true;

            if (ImGui::Button("Open")) {
                openFolderOrFile(path.string());
            }
            std::string filenameString = path.filename().string();
            /*if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "New name", filenameString)) {
                renameFile(path.filename().string(), filenameString);
            }*/
            if (ImGui::Button("LoadModel")) {
                tryLoadModel(path.string());
            }
            if (ImGui::Button("Delete")) {
                if (is_directory(path)) {
                    std::filesystem::remove_all(path);
                } else {
                    deleteFile(path.filename().string());
                }
            }
            ImGui::EndPopup();
        }
        return opened;
    }

    bool ContentBrowserPanel::openMenuPopupWindow(const std::filesystem::path& path) {
        bool opened = false;
        if (ImGui::BeginPopupContextWindow()) {
            opened = true;
            if (ImGui::BeginMenu("New")) {
                std::string newFolderName;
                /*if (ImGuiUtils::beginMenuWithInputMenuField("New Folder", "New Folder", newFolderName)) {
                    std::filesystem::create_directory(m_currentPath / newFolderName);
                }
                std::string newSceneName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Scene", "Scene name", newSceneName)) {
                    std::string scenePath = m_currentPath.string() + "\\" + newSceneName + ".scene";
                    runFunctionBasedOnFileType(scenePath,
                                               [&] {
                                                   m_client.systemsRegistry.getSystem<SceneManager>().createNewScene(scenePath);
                                               },
                                               [&] {
                                                   m_server.systemsRegistry.getSystem<SceneManager>().createNewScene(scenePath);
                                               });
                }
                std::string newScriptName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Script", "Script name", newScriptName)) {
                    TE_LOGGER_INFO("(WIP) New script name: {0}", newScriptName);
                }
                std::string newMaterialName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Material", "Material name", newMaterialName)) {
                    runFunctionBasedOnFileType(m_currentPath.string() + "\\" + newMaterialName + ".mat",
                                               [&] {
                                                   m_client.systemsRegistry.getSystem<MaterialManager>().createMaterialFile(newMaterialName, path.string());
                                               },
                                               [&] {
                                                   m_server.systemsRegistry.getSystem<MaterialManager>().createMaterialFile(newMaterialName, path.string());
                                               });
                }*/
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
        return opened;
    }

    void ContentBrowserPanel::runFunctionBasedOnFileType(std::string path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction) {
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

    FileType ContentBrowserPanel::getFileType(const std::filesystem::path& path) {
        std::string rootFolder = path.string().substr(m_editorRegistry.getSystem<ProjectManager>().getProjectPath().string().size() + std::string("\\Assets\\").size());
        rootFolder = rootFolder.substr(0, rootFolder.find_first_of('\\'));
        if (rootFolder == "client") {
            return FileType::Client;
        } else if (rootFolder == "server") {
            return FileType::Server;
        } else if (rootFolder == "common") {
            return FileType::Common;
        } else {
            TE_LOGGER_CRITICAL("Unknown file type: \n\tRoot folder: {0}\n\tPath: {1}", rootFolder, path.string());
            return FileType::Common;
        }
    }
}
