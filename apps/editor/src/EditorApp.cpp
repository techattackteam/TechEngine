#include "EditorApp.hpp"

#include <TechEngine/platform/Platform.hpp>

#include "TechEngine/base/diagnostics/Assert.hpp"
#include "TechEngine/base/diagnostics/Log.hpp"
#include "TechEngine/client/render/FrameCommand.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace TechEngine {

    EditorApp::EditorApp(std::filesystem::path projectRoot) : App(editorRole()), m_projectRoot(std::move(projectRoot)) {
    }

    EditorApp::~EditorApp() {
    }

    void EditorApp::init() {
        m_mounts.mount("project", m_projectRoot);
        m_mounts.mount("engine", executablePath().parent_path() / "assets");

        const ProjectResult loaded = m_project.load(m_files, "project://project.toml");
        TE_CHECK(loaded == ProjectResult::Ok, "Failed to load project.toml under {0} (ProjectResult {1})", m_projectRoot.string(), static_cast<int>(loaded));

        const std::filesystem::path& root = m_project.root();
        m_mounts.mount("shaders", root / "shaders");
        m_mounts.mount("assets", root / "assets" / "common", 0);
        m_mounts.mount("assets", root / "assets" / "client", 100);

        TE_LOGGER_INFO("Opened project '{0}' at {1}", m_project.name(), root.string());
        TE_CHECK(m_client.start(m_jobs, 1280, 720, "TechEngine Editor"), "Failed to start the client session");
    }

    void EditorApp::fixedUpdate(const SimulationContext&) {
    }

    void EditorApp::update(const SimulationContext& context) {
        m_client.publish(FrameCommand{
            .clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            .drawTriangle = true,
            .frameIndex = context.iterationIndex,
        });
    }
    void EditorApp::mainUpdate() {
        App::mainUpdate();
        m_client.pollEvents();
        const std::uint64_t rateSample = m_simulationThread.rateSampleIndex();
        if (rateSample != m_lastRateSample) {
            m_lastRateSample = rateSample;
            const auto framesPerSecond = static_cast<std::uint64_t>(m_client.renderFramesPerSecond());
            const auto ticksPerSecond = static_cast<std::uint64_t>(m_simulationThread.ticksPerSecond());
            m_client.setTitle("TechEngine Editor | FPS: " + std::to_string(framesPerSecond) + " | TPS: " + std::to_string(ticksPerSecond));
        }
    }

    void EditorApp::shutdown() {
        m_client.stop();
    }
    bool EditorApp::shouldClose() const {
        return m_client.shouldClose();
    }

    Role EditorApp::editorRole() {
        return Role::Client;
    }
}
