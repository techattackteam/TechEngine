#include "PanelsManager.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "core/Logger.hpp"
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <filesystem>
#include <commdlg.h>
#include <imgui_internal.h>

#include "project/ProjectManager.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/OnProcessCloseEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/window/WindowCloseEvent.hpp"

namespace TechEngine {
    PanelsManager::PanelsManager(Client& client,
                                 Server& server,
                                 EventDispatcher& eventDispatcher,
                                 ProjectManager& projectManager) : client(client),
                                                                   server(server),
                                                                   eventDispatcher(eventDispatcher),
                                                                   projectManager(projectManager),
                                                                   contentBrowser(client, server, *this, projectManager),
                                                                   exportSettingsPanel(eventDispatcher, *this, projectManager, client.sceneManager, client.renderer.getShadersManager()),
                                                                   clientPanel(client, eventDispatcher, *this, projectManager),
                                                                   serverPanel(*this, eventDispatcher, server, projectManager, client.renderer),
                                                                   materialEditor(client, server) {
    }

    void PanelsManager::init() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](TechEngine::Event* event) {
            if (clientPanel.isRunning()) {
                clientPanel.stopRunningScene();
            }
            if (serverPanel.isRunning()) {
                serverPanel.stopRunningScene();
            }
            closeAllProcessEvents();
        });
        eventDispatcher.subscribe(RegisterCustomPanel::eventType, [this](TechEngine::Event* event) {
            registerCustomPanel((RegisterCustomPanel*)event);
        });

        /*EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](TechEngine::Event* event) {
            clientPanel.sceneHierarchyPanel.deselectGO(sceneManager.getScene().getGameObjectByTag(((GameObjectDestroyEvent*)event)->getGameObjectTag()));
        });*/

        eventDispatcher.subscribe(KeyPressedEvent::eventType, [this](TechEngine::Event* event) {
            OnKeyPressedEvent(((KeyPressedEvent*)event)->getKey());
        });

        eventDispatcher.subscribe(KeyReleasedEvent::eventType, [this](Event* event) {
            OnKeyReleasedEvent(((KeyReleasedEvent*)event)->getKey());
        });

        eventDispatcher.subscribe(OnProcessCloseEvent::eventType, [this](Event* event) {
            onCloseProcessEvent(((OnProcessCloseEvent*)event)->getProcessId());
        });

        contentBrowser.init();
        materialEditor.init();
        initImGui();
    }

    void PanelsManager::update() {
        beginImGuiFrame();
        for (CustomPanel* panel: customPanels) {
            panel->update();
        }
        clientPanel.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        serverPanel.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        contentBrowser.update();
        exportSettingsPanel.update();
        materialEditor.update(ImGuiStyleVar_WindowPadding, ImVec2{0, 0}, ImGuiWindowFlags_None, true);
        endImGuiFrame();
    }

    void PanelsManager::registerCustomPanel(RegisterCustomPanel* event) {
        customPanels.emplace_back(event->getPanel());
        event->getPanel()->setupImGuiContext(imguiContext);
    }

    void PanelsManager::initImGui() {
        imguiContext = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigViewportsNoAutoMerge = true;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
        }

        setColorTheme();

        ImGui_ImplGlfw_InitForOpenGL(client.window.getHandler(), true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void PanelsManager::setColorTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Modify the blue color to make it more grayish
        colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f); // Gray text
        colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f); // Dark gray text (disabled)
        colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // Dark gray window background
        colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.30f); // Light gray border
        colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.20f); // Dark gray border shadow
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray frame background
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // Light blue frame background (hovered)
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue frame background (active)
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark gray title background
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // Dark gray title background (active)
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f); // Dark gray title background (collapsed)
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray menu bar background
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark gray scrollbar background
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // Light blue scrollbar grab
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue scrollbar grab (hovered)
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue scrollbar grab (active)
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue check mark
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f); // Blue slider grab
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue slider grab (active)
        colors[ImGuiCol_Button] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f); // Lighter gray button
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue button (hovered)
        colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue button (active)
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f); // Light blue header
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // Light blue header (hovered)
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue header (active)
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border]; // Light gray separator
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f); // Light blue separator (hovered)
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue separator (active)
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f); // Light blue resize grip
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f); // Light blue resize grip (hovered)
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Light blue resize grip (active)
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f); // Light blue tab (hovered)
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f); // Dark gray tab (active)
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab (unfocused)
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f); // Gray tab (unfocused, active)
        colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f); // Light blue docking preview
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray docking empty background

        style.WindowRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupRounding = 2.0f;
        style.ChildRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    }

    void PanelsManager::beginImGuiFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        createDockSpace();
        createMenuBar();

        ImGui::End();
    }

    void PanelsManager::createDockSpace() {
        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 170.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = minWinSizeX;
    }

    void PanelsManager::createMenuBar() {
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                projectManager.saveProject();
            }

            if (ImGui::MenuItem("Export")) {
                if (!exportSettingsPanel.isOpen()) {
                    exportSettingsPanel.open();
                }
            }
            if (ImGui::MenuItem("Exit")) {
                client.eventDispatcher.dispatch(new AppCloseRequestEvent());
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Client")) {
            if (ImGui::MenuItem("Build")) {
                /*if (clientPanel.isRunning()) {
                    clientPanel.stopRunningScene();
                }*/
#ifdef TE_DEBUG
                compileUserScripts(DEBUG, PROJECT_CLIENT);
#elif TE_RELEASE
                compileUserScripts(RELEASE, PROJECT_CLIENT);
#elif TE_RELEASEDEBUG
                compileUserScripts(RELEASEDEBUG, PROJECT_CLIENT);
#endif
            } else if (ImGui::MenuItem("Run")) {
                runClientProcess();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Server")) {
            if (ImGui::MenuItem("Build")) {
                if (!serverProcesses.empty()) {
                    for (PROCESS_INFORMATION& process: serverProcesses) {
                        TerminateProcess(process.hProcess, 0);
                        onCloseProcessEvent(process.dwProcessId);
                    }
                }
#ifdef TE_DEBUG
                compileUserScripts(DEBUG, PROJECT_SERVER);
#elif TE_RELEASE
                compileUserScripts(RELEASE, PROJECT_SERVER);
#elif TE_RELEASEDEBUG
                compileUserScripts(RELEASEDEBUG, PROJECT_SERVER);
#endif*/
            } else if (ImGui::MenuItem("Run")) {
                runServerProcess();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }


    void PanelsManager::endImGuiFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGui::EndFrame();
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    std::string PanelsManager::openFileWindow(const char* filter) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }

    std::string PanelsManager::saveFile(const char* filter) {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        CHAR currentDir[256] = {0};
        ZeroMemory(&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        if (GetCurrentDirectoryA(256, currentDir))
            ofn.lpstrInitialDir = currentDir;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        // Sets the default extension by extracting it from the filter
        ofn.lpstrDefExt = strchr(filter, '\0') + 1;

        if (GetSaveFileNameA(&ofn) == TRUE)
            return ofn.lpstrFile;

        return std::string();
    }


    void PanelsManager::compileUserScripts(CompileMode compileMode, CompileProject compileProject) {
        std::string cmakeBuildPath = compileProject == CompileProject::PROJECT_CLIENT ? projectManager.getClientCmakeBuildPath().string() : projectManager.getServerCmakeBuildPath().string();
        std::string cmakeListPath = compileProject == CompileProject::PROJECT_CLIENT ? projectManager.getClientCmakeListPath().string() : projectManager.getServerCmakeListPath().string();
        std::string techEngineLibPath = compileProject == CompileProject::PROJECT_CLIENT ? projectManager.getTechEngineClientLibPath().string() : projectManager.getTechEngineServerLibPath().string();
        std::string techEngineCoreLibPath = projectManager.getTechEngineCoreLibPath().string();

        std::string variable = compileProject == CompileProject::PROJECT_CLIENT ? "TechEngineClientLIB:STRING=\"" : "TechEngineServerLIB:STRING=\"";

        if (!std::filesystem::exists(cmakeBuildPath) || std::filesystem::is_empty(cmakeBuildPath)) {
            std::string command = "\"" + projectManager.getCmakePath().string() +
                                  " -G \"Visual Studio 17 2022\""
                                  " -D " + variable + techEngineLibPath + "\"" +
                                  " -D TechEngineCoreLIB:STRING=\"" + techEngineCoreLibPath + "\"" +
                                  " -S " + "\"" + cmakeListPath + "\"" +
                                  " -B " + "\"" + cmakeBuildPath + "\"" + "\"";
            std::system(command.c_str());
        }
        std::string cm;
        if (compileMode == CompileMode::RELEASE) {
            cm = "Release";
        } else if (compileMode == CompileMode::RELEASEDEBUG) {
            cm = "RelWithDebInfo";
        } else if (compileMode == CompileMode::DEBUG) {
            cm = "Debug";
        }
        std::string command = "\"" + projectManager.getCmakePath().string() +
                              " --build " + "\"" + cmakeBuildPath + "\"" + " --target UserScripts --config " + cm + "\"";
        std::system(command.c_str());
        TE_LOGGER_INFO("Build finished!");
    }

    void closeRunningProcesses(PVOID lpParameter, BOOLEAN TimerOrWaitFiredm) {
        DWORD dwProcessId = reinterpret_cast<DWORD>(lpParameter);
        //client.eventDispatcher.dispatch(new OnProcessCloseEvent(dwProcessId));
    }

    void PanelsManager::runServerProcess() {
#ifdef TE_DEBUG
        exportSettingsPanel.exportServerProject(DEBUG);
#else
        exportSettingsPanel.exportServerProject(RELEASEDEBUG);
#endif
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        std::string path = projectManager.getProjectCachePath().string() + "\\Server\\TechEngineServer.exe";
        std::string currentDirectory = projectManager.getProjectCachePath().string() + "\\Server";
        if (!CreateProcessA(
            nullptr, // Application name (we can specify the engine executable here to launch a copy)
            (LPSTR)path.c_str(), // Command line argument: the DLL path
            nullptr, // Process security attributes
            nullptr, // Thread security attributes
            FALSE, // Do not inherit handles
            CREATE_NEW_CONSOLE, // Creation flags
            nullptr, // Environment variables
            currentDirectory.c_str(), // Current directory
            &si, // Startup info
            &pi // Process information
        )) {
            TE_LOGGER_ERROR("Failed to create process for TechEngineServer.\nError code: {0}", GetLastError());
            return;
        }
        serverProcesses.push_back(pi);
        HANDLE hWaitHandle;
        if (!RegisterWaitForSingleObject(
            &hWaitHandle,
            pi.hProcess, // Handle to wait on
            &closeRunningProcesses, // Callback function
            reinterpret_cast<void*>(pi.dwProcessId), // Context passed to the callback
            INFINITE, // Timeout
            WT_EXECUTEONLYONCE // Trigger callback only once
        )) {
            TE_LOGGER_ERROR("Failed to register wait for single object");
        }
    }

    void PanelsManager::runClientProcess() {
#ifdef TE_DEBUG
        exportSettingsPanel.exportGameProject(DEBUG);
#else
        exportSettingsPanel.exportGameProject(RELEASEDEBUG);
#endif
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        std::string path = projectManager.getProjectCachePath().string() + "\\Client\\TechEngineRuntime.exe";
        std::string currentDirectory = projectManager.getProjectCachePath().string() + "\\Client";
        if (!CreateProcessA(
            nullptr, // Application name (we can specify the engine executable here to launch a copy)
            (LPSTR)path.c_str(), // Command line argument: the EXE path
            nullptr, // Process security attributes
            nullptr, // Thread security attributes
            TRUE, // Do not inherit handles
            CREATE_NEW_CONSOLE, // Creation flags
            nullptr, // Environment variables
            (LPSTR)currentDirectory.c_str(), // Current directory
            &si, // Startup info
            &pi // Process information
        )) {
            TE_LOGGER_ERROR("Failed to create process for TechEngineRuntime.\nError code: {0}", GetLastError());
            return;
        }
        clientProcesses.push_back(pi);
        HANDLE hWaitHandle;
        if (!RegisterWaitForSingleObject(
            &hWaitHandle,
            pi.hProcess, // Handle to wait on
            &closeRunningProcesses, // Callback function
            reinterpret_cast<void*>(pi.dwProcessId), // Context passed to the callback
            INFINITE, // Timeout
            WT_EXECUTEONLYONCE // Trigger callback only once
        )) {
            TE_LOGGER_ERROR("Failed to register wait for single object");
        }
    }

    void PanelsManager::onCloseProcessEvent(DWORD processId) {
        for (auto it = clientProcesses.begin(); it != clientProcesses.end(); it++) {
            if (it->dwProcessId == processId) {
                clientProcesses.erase(it);
                return;
            }
        }
        for (auto it = serverProcesses.begin(); it != serverProcesses.end(); it++) {
            if (it->dwProcessId == processId) {
                serverProcesses.erase(it);
                return;
            }
        }
    }

    void PanelsManager::closeAllProcessEvents() {
        for (auto it = clientProcesses.begin(); it != clientProcesses.end(); it++) {
            TerminateProcess(it->hProcess, 0);
        }
        for (auto it = serverProcesses.begin(); it != serverProcesses.end(); it++) {
            TerminateProcess(it->hProcess, 0);
        }
    }

    void PanelsManager::OnKeyPressedEvent(Key& key) {
        if (key.getKeyCode() == LEFT_CTRL || key.getKeyCode() == RIGHT_CTRL) {
            isCtrlPressed = true;
        } else if (isCtrlPressed && key.getKeyCode() == S) {
            projectManager.saveProject();
        }
    }

    void PanelsManager::OnKeyReleasedEvent(Key& key) {
        if (key.getKeyCode() == LEFT_CTRL || key.getKeyCode() == RIGHT_CTRL) {
            isCtrlPressed = false;
        }
    }

    /*std::vector<GameObject*>& PanelsManager::getSelectedGameObjects() {
        return sceneHierarchyPanel.getSelectedGO();
    }*/

    void PanelsManager::openMaterialEditor(const std::string& materialName, const std::string& filepath) {
        materialEditor.open(materialName, filepath);
    }
}
