#include "PanelsManager.hpp"
#include "project/ProjectManager.hpp"
#include "ContentBrowserPanel.hpp"
#include "core/Logger.hpp"
#include "UIUtils/ImGuiUtils.hpp"

namespace TechEngine {
    ContentBrowserPanel::ContentBrowserPanel(Client& client,
                                             Server& server,
                                             PanelsManager& panelsManager,
                                             ProjectManager& projectManager) : client(client),
                                                                               server(server),
                                                                               panelsManager(panelsManager),
                                                                               projectManager(projectManager),
                                                                               /*assimpLoader(/*sceneManager.getScene(), materialManager#1#),*/
                                                                               Panel("Content Browser") {
    }


    void ContentBrowserPanel::init() {
        currentPath = projectManager.getProjectAssetsPath();
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
            renderDirectoryHierarchy(projectManager.getProjectCommonAssetsPath().string());
            renderDirectoryHierarchy(projectManager.getProjectClientAssetsPath().string());
            renderDirectoryHierarchy(projectManager.getProjectServerAssetsPath().string());
            ImGui::PopStyleVar();

            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);

            ImGui::BeginChild("Files", {ImGui::GetContentRegionAvail().x, ImGui::GetWindowSize().y});
            renderFiles(currentPath);
            ImGui::EndChild();
            ImGui::EndTable();
        }
    }

    void ContentBrowserPanel::renderDirectoryHierarchy(const std::filesystem::path& directoryPath) {
        ImGuiTreeNodeFlags flags = ((currentPath == directoryPath) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow |
                                   ImGuiTreeNodeFlags_SpanFullWidth |
                                   ImGuiTreeNodeFlags_OpenOnArrow;
        std::string relativePath = directoryPath.string().substr(directoryPath.string().find_last_of('\\') + 1);
        bool open = ImGui::TreeNodeEx(relativePath.c_str(), flags);
        if (ImGui::IsItemClicked()) {
            currentPath = directoryPath;
            selectedPath = directoryPath;
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
            auto relativePath = std::filesystem::relative(path, projectManager.getProjectLocation());
            std::string filenameString = relativePath.filename().string();
            bool hasSubDirs = false;

            for (auto& subDir: std::filesystem::directory_iterator(path)) {
                if (subDir.is_directory()) {
                    hasSubDirs = true;
                    break;
                }
            }

            // Define flags based on whether the node is selected or not
            ImGuiTreeNodeFlags flags = ((currentPath == path) ? ImGuiTreeNodeFlags_Selected : 0) |
                                       ImGuiTreeNodeFlags_OpenOnArrow |
                                       (hasSubDirs ? 0 : ImGuiTreeNodeFlags_Leaf) |
                                       ImGuiTreeNodeFlags_SpanFullWidth;


            bool open = ImGui::TreeNodeEx(filenameString.c_str(), flags);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                currentPath = path;
                selectedPath = path;
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
            auto relativePath = std::filesystem::relative(path, projectManager.getProjectAssetsPath());
            std::string filenameString = relativePath.filename().string();
            if (selectedPath == path.string()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            }
            ImGui::Button(filenameString.c_str());
            if (selectedPath == path.string()) {
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
                    selectedPath = path.string();
                }
            }
            if (openMenuPopupItem(path)) {
                opened = true;
            }
        }
        if (!opened) {
            if (openMenuPopupWindow(currentPath)) {
                opened = true;
            }
        }
    }

    void ContentBrowserPanel::openFolderOrFile(const std::string& path) {
        if (std::filesystem::is_directory(path)) {
            currentPath = path;
        } else {
            std::string filename = path.substr(path.find_last_of('\\') + 1);
            std::string extension = filename.substr(filename.find_last_of('.'));
            if (extension == ".scene") {
                std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
                runFunctionBasedOnFileType(path,
                                           [&] {
                                               client.sceneManager.loadScene(filenameWithoutExtension);
                                           },
                                           [&] {
                                               server.sceneManager.loadScene(filenameWithoutExtension);
                                           }
                );
            } else if (extension == ".mat") {
                std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
                panelsManager.openMaterialEditor(filenameWithoutExtension, path);
            }
        }
    }

    void ContentBrowserPanel::renameFile(const std::string& filename, const std::string& newName) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        std::filesystem::rename(currentPath / filename, currentPath / (newName + extension));
    }

    void ContentBrowserPanel::tryLoadModel(const std::string& filepath) {
        //assimpLoader.loadModel(filepath);
    }

    void ContentBrowserPanel::deleteFile(const std::string& filename) {
        std::string extension = filename.substr(filename.find_last_of('.'));
        if (extension == ".scene") {
            std::string path = currentPath.string() + "\\" + filename;

            if (client.sceneManager.hasScene(path)) {
                client.sceneManager.deleteScene(path);
                std::filesystem::remove(path);
            } else if (server.sceneManager.hasScene(path)) {
                server.sceneManager.deleteScene(path);
                std::filesystem::remove(path);
            }
        } else if (extension == ".mat") {
            std::string path = currentPath.string() + "\\" + filename;
            std::string filenameWithoutExtension = filename.substr(0, filename.find_last_of('.'));
            if (client.materialManager.materialExists(filenameWithoutExtension)) {
                client.materialManager.deleteMaterial(filenameWithoutExtension);
                std::filesystem::remove(path);
            } else if (server.materialManager.materialExists(filenameWithoutExtension)) {
                server.materialManager.deleteMaterial(filenameWithoutExtension);
                std::filesystem::remove(path);
            }
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
            if (ImGuiUtils::beginMenuWithInputMenuField("Rename", "New name", filenameString)) {
                renameFile(path.filename().string(), filenameString);
            }
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
                if (ImGuiUtils::beginMenuWithInputMenuField("New Folder", "New Folder", newFolderName)) {
                    std::filesystem::create_directory(currentPath / newFolderName);
                }
                std::string newSceneName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Scene", "Scene name", newSceneName)) {
                    std::string scenePath = currentPath.string() + "\\" + newSceneName + ".scene";
                    runFunctionBasedOnFileType(scenePath,
                                               [&] {
                                                   client.sceneManager.createNewScene(scenePath);
                                               },
                                               [&] {
                                                   server.sceneManager.createNewScene(scenePath);
                                               });
                }
                std::string newScriptName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Script", "Script name", newScriptName)) {
                    TE_LOGGER_INFO("(WIP) New script name: {0}", newScriptName);
                }
                std::string newMaterialName;
                if (ImGuiUtils::beginMenuWithInputMenuField("New Material", "Material name", newMaterialName)) {
                    runFunctionBasedOnFileType(currentPath.string() + "\\" + newMaterialName + ".mat",
                                               [&] {
                                                   client.materialManager.createMaterialFile(newMaterialName, path.string());
                                               },
                                               [&] {
                                                   server.materialManager.createMaterialFile(newMaterialName, path.string());
                                               });
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
        return opened;
    }

    void ContentBrowserPanel::runFunctionBasedOnFileType(std::string path, const std::function<void()>& clientFunction, const std::function<void()>& serverFunction) {
        FileType fileType = getFileType(path);
        if (fileType == FileType::CLIENT) {
            clientFunction();
        } else if (fileType == FileType::SERVER) {
            serverFunction();
        } else if (fileType == FileType::COMMON) {
            clientFunction();
            serverFunction();
        }
    }

    FileType ContentBrowserPanel::getFileType(const std::filesystem::path& path) {
        std::string rootFolder = path.string().substr(projectManager.getProjectLocation().string().size() + std::string("\\Assets\\").size());
        rootFolder = rootFolder.substr(0, rootFolder.find_first_of('\\'));
        if (rootFolder == "Client") {
            return FileType::CLIENT;
        } else if (rootFolder == "Server") {
            return FileType::SERVER;
        } else if (rootFolder == "Common") {
            return FileType::COMMON;
        } else {
            TE_LOGGER_CRITICAL("Unknown file type: \n\tRoot folder: {0}\n\tPath: {1}", rootFolder, path.string());
            return FileType::COMMON;
        }
    }
}
