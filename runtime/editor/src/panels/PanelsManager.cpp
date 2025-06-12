#include "PanelsManager.hpp"


#include "DockPanel.hpp"
#include "core/Client.hpp"
#include "core/Server.hpp"
#include "systems/SystemsRegistry.hpp"

#include "events/application/AppCloseEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"

#include "script/ScriptEngine.hpp"
#include "scripting/ScriptsCompiler.hpp"

#include "window/Window.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <imgui_internal.h>

namespace TechEngine {
    PanelsManager::PanelsManager(SystemsRegistry& systemsRegistry,
                                 Client& client,
                                 Server& server) : m_systemsRegistry(systemsRegistry),
                                                   m_client(client), m_server(server),
                                                   m_clientPanel(systemsRegistry, m_client.m_systemRegistry,
                                                                 m_LoggerPanel),
                                                   m_serverPanel(systemsRegistry, m_server.m_systemRegistry,
                                                                 m_LoggerPanel),
                                                   m_LoggerPanel(systemsRegistry,
                                                                 m_client.m_systemRegistry,
                                                                 m_server.m_systemRegistry),
                                                   m_ContentBrowserPanel(systemsRegistry,
                                                                         m_client.m_systemRegistry,
                                                                         m_server.m_systemRegistry, *this) {
    }


    void PanelsManager::init() {
        initImGui();
        m_clientPanel.init("Client Panel", &m_editorWindowClass);
        m_serverPanel.init("Server Panel", &m_editorWindowClass);
        m_LoggerPanel.init("Logger", &m_editorWindowClass);
        m_ContentBrowserPanel.init("Content Browser", &m_editorWindowClass);
    }

    void PanelsManager::onUpdate() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        m_dockSpaceID = ImGui::GetID("EditorDockSpace");
        createDockSpace();
        m_clientPanel.update();
        m_serverPanel.update();
        m_LoggerPanel.update();
        m_ContentBrowserPanel.update();
        static bool firstTime = true;
        if (firstTime) {
            if (!std::filesystem::exists("imgui.ini")) {
                setupInitialDockingLayout();
            }
            firstTime = false;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    void PanelsManager::shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void PanelsManager::initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
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
        ImGui_ImplGlfw_InitForOpenGL(m_systemsRegistry.getSystem<Window>().getHandler(), true);
        ImGui_ImplOpenGL3_Init();
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
        colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f); // Dark gray title background
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // Dark gray title background (active)
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        // Dark gray title background (collapsed)
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Gray menu bar background
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark gray scrollbar background
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f); // Light blue scrollbar grab
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        // Light blue scrollbar grab (hovered)
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
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

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

    void PanelsManager::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID);
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_dockSpaceID, ImGui::GetMainViewport()->Size);

        ImGuiID dockMainID = m_dockSpaceID;
        ImGuiID dockButtonID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Down, 0.2f, nullptr, &dockMainID);

        ImGui::DockBuilderDockWindow((m_clientPanel.getName() + "##" + std::to_string(m_clientPanel.getId())).c_str(),
                                     dockMainID);
        ImGui::DockBuilderDockWindow((m_serverPanel.getName() + "##" + std::to_string(m_serverPanel.getId())).c_str(),
                                     dockMainID);
        ImGui::DockBuilderDockWindow(
            (m_ContentBrowserPanel.getName() + "##" + std::to_string(m_ContentBrowserPanel.getId())).c_str(),
            dockButtonID);
        ImGui::DockBuilderDockWindow((m_LoggerPanel.getName() + "##" + std::to_string(m_LoggerPanel.getId())).c_str(),
                                     dockButtonID);

        ImGui::DockBuilderFinish(m_dockSpaceID);
    }


    void PanelsManager::createDockSpace() {
        // Note: Switch this to true to enable dockspace
        static bool dockspaceOpen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGuiID dockSpaceId = ImGui::GetID("EditorDockSpace");
        m_editorWindowClass.ClassId = dockSpaceId;
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 170.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockspace_flags, &m_editorWindowClass);
        }
        style.WindowMinSize.x = minWinSizeX;
        drawMenuBar();
        ImGui::End();
    }

    void PanelsManager::drawMenuBar() {
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
            }
            if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                //m_systemsRegistry.getSystem<ProjectManager>().saveProject();
            }

            if (ImGui::MenuItem("Export")) {
                /*if (!exportSettingsPanel.isOpen()) {
                    exportSettingsPanel.open();
                }*/
                m_systemsRegistry.getSystem<ProjectManager>().exportProject(
                    "C:\\dev\\TechEngine\\bin\\runtime\\editor\\Client", ProjectType::Client);
                m_systemsRegistry.getSystem<ProjectManager>().exportProject(
                    "C:\\dev\\TechEngine\\bin\\runtime\\editor\\Server", ProjectType::Server);
            }
            if (ImGui::MenuItem("Exit")) {
                m_systemsRegistry.getSystem<EventDispatcher>().dispatch<AppCloseEvent>();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Client")) {
            if (ImGui::MenuItem("Build")) {
                /*if (clientPanel.isRunning()) {
                    clientPanel.stopRunningScene();
                }*/
                ScriptsCompiler::compileUserScripts(m_systemsRegistry.getSystem<ProjectManager>(), CompileMode::Release,
                                                    ProjectType::Client);
            } else if (ImGui::MenuItem("Run")) {
                //runClientProcess();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Server")) {
            if (ImGui::MenuItem("Build")) {
                /*if (!serverProcesses.empty()) {
                    for (PROCESS_INFORMATION& process: serverProcesses) {
                        TerminateProcess(process.hProcess, 0);
                        onCloseProcessEvent(process.dwProcessId);
                    }
                }*/
                ScriptsCompiler::compileUserScripts(m_systemsRegistry.getSystem<ProjectManager>(), CompileMode::Debug,
                                                    ProjectType::Client);
            } else if (ImGui::MenuItem("Run")) {
                //runServerProcess();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}
