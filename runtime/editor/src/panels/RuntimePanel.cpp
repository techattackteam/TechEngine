#include "RuntimePanel.hpp"

#include "systems/SystemsRegistry.hpp"
#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"
#include "scripting/ScriptsCompiler.hpp"
#include "simulator/RuntimeSimulator.hpp"
#include "LoggerPanel.hpp"
#include "core/Client.hpp"
#include "core/Server.hpp"

#include <filesystem>
#include <imgui_internal.h>

namespace TechEngine {
    RuntimePanel::RuntimePanel(SystemsRegistry& editorSystemsRegistry,
                               SystemsRegistry& appSystemsRegistry,
                               LoggerPanel& loggerPanel): m_editorSystemsRegistry(editorSystemsRegistry),
                                                          m_appSystemsRegistry(appSystemsRegistry),
                                                          m_inspectorPanel(editorSystemsRegistry, appSystemsRegistry, m_selectedEntities),
                                                          m_sceneHierarchyPanel(editorSystemsRegistry, appSystemsRegistry, m_selectedEntities),
                                                          m_gameView(editorSystemsRegistry, appSystemsRegistry),
                                                          m_sceneView(editorSystemsRegistry, appSystemsRegistry, m_selectedEntities),
                                                          loggerPanel(loggerPanel), DockPanel(editorSystemsRegistry) {
    }

    void RuntimePanel::onInit() {
        m_sceneView.init("Scene View", &m_dockSpaceWindowClass);
        m_gameView.init("Game View", &m_dockSpaceWindowClass);
        m_sceneHierarchyPanel.init("Scene Hierarchy", &m_dockSpaceWindowClass);
        m_inspectorPanel.init("Inspector", &m_dockSpaceWindowClass);
    }

    void RuntimePanel::onUpdate() {
        m_sceneView.update();
        m_gameView.update();
        m_sceneHierarchyPanel.update();
        m_inspectorPanel.update();
    }

    void RuntimePanel::setupInitialDockingLayout() {
        ImGui::DockBuilderRemoveNode(m_dockSpaceID);
        ImGui::DockBuilderAddNode(m_dockSpaceID, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(m_dockSpaceID, ImGui::GetMainViewport()->Size);

        ImGuiID dockMainID = m_dockSpaceID;
        ImGuiID dockRightID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.2f, nullptr, &dockMainID);
        ImGuiID dockLeftID = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, 0.2f, nullptr, &dockMainID);

        ImGui::DockBuilderDockWindow((m_sceneView.getName() + "##" + std::to_string(m_sceneView.getId())).c_str(), dockMainID);
        ImGui::DockBuilderDockWindow((m_gameView.getName() + "##" + std::to_string(m_gameView.getId())).c_str(), dockMainID);
        ImGui::DockBuilderDockWindow((m_sceneHierarchyPanel.getName() + "##" + std::to_string(m_sceneHierarchyPanel.getId())).c_str(), dockRightID);
        ImGui::DockBuilderDockWindow((m_inspectorPanel.getName() + "##" + std::to_string(m_inspectorPanel.getId())).c_str(), dockLeftID);

        ImGui::DockBuilderFinish(m_dockSpaceID);
    }

    void RuntimePanel::startRunningScene() {
        ProjectManager& projectManager = m_editorSystemsRegistry.getSystem<ProjectManager>();
        ScriptsCompiler::compileUserScripts(projectManager, CompileMode::Debug, m_projectType);
        std::string dllPath = projectManager.getResourcesPath().string() + (m_projectType == ProjectType::Client ? "\\client\\scripts\\build\\debug\\ClientScripts.dll" : "\\server\\scripts\\build\\debug\\ServerScripts.dll");
        if (ProjectType::Client == m_projectType) {
            if (!m_editorSystemsRegistry.getSystem<RuntimeSimulator<Client>>().startSimulation(dllPath, loggerPanel.m_sink)) {
                stopRunningScene();
            }
        } else {
            if (!m_editorSystemsRegistry.getSystem<RuntimeSimulator<Server>>().startSimulation(dllPath, loggerPanel.m_sink)) {
                stopRunningScene();
            }
        }
    }

    void RuntimePanel::stopRunningScene() {
        m_editorSystemsRegistry.getSystem<RuntimeSimulator<Client>>().stopSimulation();
    }

    void RuntimePanel::createToolBar() {
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
        if (m_editorSystemsRegistry.getSystem<RuntimeSimulator<Client>>().getSimulationState() == SimulationState::RUNNING) {
            playStopText = "Stop";
        } else if (m_editorSystemsRegistry.getSystem<RuntimeSimulator<Client>>().getSimulationState() == SimulationState::STOPPED) {
            playStopText = "Play";
        }
        if (ImGui::Button(playStopText.c_str(), ImVec2(size, 0))) {
            if (m_editorSystemsRegistry.getSystem<RuntimeSimulator<Client>>().getSimulationState() == SimulationState::STOPPED) {
                startRunningScene();
            } else {
                stopRunningScene();
            }
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos.x / 2 + (size / 2));
        if (ImGui::Button("Add Physics Bodies", ImVec2(size, 0))) {
            m_appSystemsRegistry.getSystem<PhysicsEngine>().start();
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove Physics Bodies", ImVec2(size, 0))) {
            m_appSystemsRegistry.getSystem<PhysicsEngine>().stop();
        }
        ImGui::SameLine();
        if (ImGui::Button("Update Bodies", ImVec2(size, 0))) {
            m_appSystemsRegistry.getSystem<PhysicsEngine>().updateBodies();
        }
        ImGui::PopStyleColor(3);
    }
}
