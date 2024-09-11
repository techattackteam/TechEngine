#include "ServerPanel.hpp"

#include "systems/SystemsRegistry.hpp"

#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"
#include "scripting/ScriptsCompiler.hpp"
#include "simulator/RuntimeSimulator.hpp"
#include "LoggerPanel.hpp"
#include "core/Server.hpp"

#include <filesystem>
#include <imgui_internal.h>

namespace TechEngine {
    ServerPanel::ServerPanel(SystemsRegistry& editorSystemsRegistry,
                             SystemsRegistry& serverSystemsRegistry,
                             LoggerPanel& loggerPanel) : m_systemRegistry(editorSystemsRegistry),
                                                         m_serverSystemsRegistry(serverSystemsRegistry),
                                                         loggerPanel(loggerPanel),
                                                         DockPanel(editorSystemsRegistry) {
    }

    void ServerPanel::onInit() {
    }

    void ServerPanel::onUpdate() {
    }

    void ServerPanel::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID); // Clear any previous layout
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace); // Create a new dockspace node

        ImGuiID test_A = ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);


        ImGui::DockBuilderFinish(m_dockSpaceID); // Finalize the layout
    }

    void ServerPanel::startRunningScene() {
        ProjectManager& projectManager = m_systemRegistry.getSystem<ProjectManager>();
        ScriptsCompiler::compileUserScripts(projectManager, CompileMode::Debug, ProjectType::Server);
        ScriptEngine& scriptEngine = m_serverSystemsRegistry.getSystem<ScriptEngine>();
        spdlog::sinks::dist_sink_mt* userDllSink;
        bool result;
        std::tie(result, userDllSink) = scriptEngine.loadDLL(projectManager.getResourcesPath().string() + "\\server\\scripts\\build\\debug\\ServerScripts.dll");
        if (!result) {
            TE_LOGGER_CRITICAL("Failed to load server scripts dll");
        }
        userDllSink->add_sink(loggerPanel.m_sink);
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().startSimulation();
    }

    void ServerPanel::stopRunningScene() {
        m_systemRegistry.getSystem<RuntimeSimulator<Server>>().stopSimulation();
        ScriptEngine& scriptEngine = m_serverSystemsRegistry.getSystem<ScriptEngine>();
        scriptEngine.stop();
    }

    void ServerPanel::createToolBar() {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));


        float size = ImGui::GetWindowWidth() / 10;
        ImVec2 pos = ImGui::GetContentRegionAvail();
        ImVec2 cursor = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(pos.x / 4 - size / 2);
        ImGui::SetCursorPosY(cursor.y + 4);
        if (ImGui::Button("World", ImVec2(size, 0))) {
            //sceneView.changeGuizmoMode(sceneView.getGuizmoMode() == ImGuizmo::MODE::WORLD ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD);
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos.x / 2 - (size / 2));
        std::string playStopText;
        if (m_systemRegistry.getSystem<RuntimeSimulator<Server>>().getSimulationState() == SimulationState::RUNNING) {
            playStopText = "Stop";
        } else if (m_systemRegistry.getSystem<RuntimeSimulator<Server>>().getSimulationState() == SimulationState::STOPPED) {
            playStopText = "Play";
        }
        if (ImGui::Button(playStopText.c_str(), ImVec2(size, 0))) {
            if (m_systemRegistry.getSystem<RuntimeSimulator<Server>>().getSimulationState() == SimulationState::STOPPED) {
                startRunningScene();
            } else {
                stopRunningScene();
            }
        }
        ImGui::PopStyleColor(3);
    }
}
