#include "ExportSettingsPanel.hpp"
#include "components/network/NetworkSync.hpp"
#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "PanelsManager.hpp"
#include <imgui.h>
#include <filesystem>
#include <yaml-cpp/emitter.h>
#include <fstream>

namespace TechEngine {
    ExportSettingsPanel::ExportSettingsPanel(SystemsRegistry& editorRegistry,
                                             Client& client,
                                             Server& server,
                                             PanelsManager& panelsManager) : editorRegistry(editorRegistry), panelsManager(panelsManager), client(client), server(server),
                                                                             Panel("ExportSettingsPanel", editorRegistry) {
        m_open = false;
    }

    ExportSettingsPanel::~ExportSettingsPanel() {
    }

    void ExportSettingsPanel::init() {
        Panel::init();
    }

    void ExportSettingsPanel::onUpdate() {
        ImGui::Text("Resolution:");

        ImGui::Text("   Width ");
        ImGui::SameLine();
        ImGui::DragInt("##Width", &width, 1, 0, 0);
        ImGui::Text("   Height ");
        ImGui::SameLine();
        ImGui::DragInt("##Height", &height, 1, 0, 0);


        if (ImGui::Button("Export")) {
            exportGameProject(RELEASE);
        }
        if (ImGui::Button("Close")) {
            m_open = false;
        };
    }

    void ExportSettingsPanel::exportGameProject(CompileMode compileMode) {
        ProjectManager& projectManager = editorRegistry.getSystem<ProjectManager>();
        try {
            std::filesystem::path exportPath;
            if (compileMode == RELEASE) { // Happens when building the game to build path. If in debug engines is in debug mode
                if (!std::filesystem::exists(projectManager.getProjectLocation().string() + "\\Build"))
                    std::filesystem::create_directory(projectManager.getProjectLocation().string() + "\\Build");
                exportPath /*= projectManager.getProjectGameExportPath()*/;
            } else if (compileMode == DEBUG || compileMode == RELEASEDEBUG) { // Happens when editor launches a new process to run the game
                if (!std::filesystem::exists(projectManager.getProjectCachePath())) {
                    std::filesystem::create_directory(projectManager.getProjectCachePath());
                }
                exportPath = projectManager.getProjectCachePath().string() + "\\Client";
            } else {
                TE_LOGGER_ERROR("Export Game: Invalid compile mode");
                return;
            }
            std::filesystem::remove_all(exportPath);
            std::filesystem::create_directory(exportPath);
            std::filesystem::create_directory(exportPath.string() + "\\Assets");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\Server");
            std::filesystem::create_directory(exportPath.string() + "\\Resources");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\Server");
            client.systemsRegistry.getSystem<SceneManager>().saveCurrentScene();
            std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                        std::filesystem::copy_options::overwrite_existing;

            std::filesystem::copy(projectManager.getProjectFilePath(), exportPath, copyOptions);
            /*
            if (!FileSystem::getAllFilesWithExtension(projectManager.getProjectAssetsPath().string() + "\\Client", {".cpp", ".hpp"}, true).empty() ||
                !FileSystem::getAllFilesWithExtension(projectManager.getProjectAssetsPath().string() + "\\Common", {".cpp", ".hpp"}, true).empty()) {
                panelsManager.compileClientUserScripts(compileMode);
            }
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath().string() + "\\Common", exportPath.string() + "\\Assets\\Common", {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath().string() + "\\Client", exportPath.string() + "\\Assets\\Client", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::create_directory(exportPath.string() + "\\Resources");
            FileSystem::copyRecursive(projectManager.getProjectCommonResourcesPath(), exportPath.string() + "\\Resources\\Common", {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(projectManager.getProjectClientResourcesPath(), exportPath.string() + "\\Resources\\Client", {".cpp", ".hpp"}, {"cmake"});
            */
            std::filesystem::copy(FileSystem::runtimePath, exportPath, copyOptions);
            TE_LOGGER_INFO("Project exported to: {0}", exportPath.string());
        } catch (std::filesystem::filesystem_error& e) {
            TE_LOGGER_ERROR("Error while exporting project: {0}", e.what());
            return;
        }
    }

    void ExportSettingsPanel::exportServerProject(CompileMode compileMode) {
        ProjectManager& projectManager = editorRegistry.getSystem<ProjectManager>();
        try {
            std::filesystem::path exportPath;
            if (compileMode == RELEASE) { // Happens when building the game to build path. If in debug engines is in debug mode
                if (!std::filesystem::exists(projectManager.getProjectLocation().string() + "\\Build")) {
                    std::filesystem::create_directory(projectManager.getProjectLocation().string() + "\\Build");
                }
                exportPath/* = projectManager.getProjectServerExportPath()*/;
            } else if (compileMode == DEBUG || compileMode == RELEASEDEBUG) { // Happens when editor launches a new process to run the game
                if (!std::filesystem::exists(projectManager.getProjectCachePath())) {
                    std::filesystem::create_directory(projectManager.getProjectCachePath());
                }
                exportPath = projectManager.getProjectCachePath().string() + "\\Server";
            } else {
                TE_LOGGER_ERROR("Export Game: Invalid compile mode");
                return;
            }
            std::filesystem::remove_all(exportPath);
            std::filesystem::create_directory(exportPath);

            std::filesystem::create_directory(exportPath.string() + "\\Assets");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Assets\\Server");
            std::filesystem::create_directory(exportPath.string() + "\\Resources");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\Common");
            std::filesystem::create_directory(exportPath.string() + "\\Resources\\Server");
            server.systemsRegistry.getSystem<SceneManager>().saveCurrentScene();
            for (auto gameObject: server.systemsRegistry.getSystem<SceneManager>().getScene().getGameObjects()) {
                if (!gameObject->hasComponent<NetworkSync>()) {
                    TE_LOGGER_WARN("GameObject {0} doesn't have NetworkSync component. It wont sync.", gameObject->getName());
                }
            }
            std::filesystem::copy_options copyOptions = std::filesystem::copy_options::recursive |
                                                        std::filesystem::copy_options::overwrite_existing;

            std::filesystem::copy(projectManager.getProjectFilePath(), exportPath, copyOptions);
            /*
            if (!FileSystem::getAllFilesWithExtension(projectManager.getProjectAssetsPath().string() + "\\Server", {".cpp", ".hpp"}, true).empty() ||
                !FileSystem::getAllFilesWithExtension(projectManager.getProjectAssetsPath().string() + "\\Common", {".cpp", ".hpp"}, true).empty()) {
                panelsManager.compileServerUserScripts(compileMode);
            }
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath().string() + "\\Server", exportPath.string() + "\\Assets\\Server", {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(projectManager.getProjectAssetsPath().string() + "\\Common", exportPath.string() + "\\Assets\\Common", {".cpp", ".hpp"}, {"cmake"});
            std::filesystem::create_directory(exportPath.string() + "\\Resources");
            FileSystem::copyRecursive(projectManager.getProjectCommonResourcesPath(), exportPath.string() + "\\Resources\\Common", {".cpp", ".hpp"}, {"cmake"});
            FileSystem::copyRecursive(projectManager.getProjectServerResourcesPath(), exportPath.string() + "\\Resources\\Server", {".cpp", ".hpp"}, {"cmake"});
            */
            std::filesystem::copy(FileSystem::serverPath, exportPath, copyOptions);
            TE_LOGGER_INFO("Project exported to: {0}", exportPath.string());
        } catch (std::filesystem::filesystem_error& e) {
            TE_LOGGER_ERROR("Error while exporting project: {0}", e.what());
            return;
        }
    }
}
