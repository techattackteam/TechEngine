#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/client/render/RenderSnapshot.hpp>
#include <TechEngine/platform/Platform.hpp>

#include <EditorApp.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace TechEngine {
    EditorApp::EditorApp(std::filesystem::path projectRoot) : App(editorRole()), m_projectRoot(std::move(projectRoot)) {
    }

    void EditorApp::init() {
        m_mounts.mount("project", m_projectRoot);
        m_mounts.mount("engine", executablePath().parent_path() / "assets");
        const std::error_code loadError = m_project.load(m_files, "project://project.toml");
        TE_CHECK(!loadError, "Failed to load project.toml under {0}: {1}", m_projectRoot.string(), loadError.message());
        const auto& root = m_project.root();
        m_mounts.mount("shaders", root / "shaders");
        m_mounts.mount("assets", root / "assets" / "common", 0);
        m_mounts.mount("assets", root / "assets" / "client", 100);
        TE_LOGGER_INFO("Opened project '{0}' at {1}", m_project.name(), root.string());
        const std::error_code startError = m_client.start(m_engine, m_input, 1280, 720, "TechEngine Editor", [this] {
            requestStop();
        });
        if (startError) {
            throw std::runtime_error{"Failed to start the client session: " + startError.message()};
        }
    }

    void EditorApp::publishSnapshot(const SimulationContext& simulation) {
        m_client.publish(RenderSnapshot{
            .clearColor = {0.1F, 0.1F, 0.1F, 1.0F},
            .drawTriangle = true,
            .tick = simulation.tick,
            .tickTime = simulation.tickTime,
            .fixedDeltaTime = simulation.fixedDeltaTime,
            .timeline = simulation.timeline,
            .input = simulation.input.held,
        });
    }

    void EditorApp::mainThreadUpdate() {
        if (stopRequested()) {
            return;
        }
        {
            TE_PROFILER_SCOPE("Main.WaitEvents");
            m_client.waitEvents();
        }
        const auto metrics = timingMetrics();
        const auto fps = static_cast<std::uint64_t>(metrics.renderingActive ? metrics.render.framesPerSecond : 0.0);
        const auto tps = static_cast<std::uint64_t>(metrics.simulation.ticksPerSecond);
        std::string title = "TechEngine Editor | FPS: " + std::to_string(fps) + " | TPS: " + std::to_string(tps);
        if (title != m_appliedTitle) {
            m_client.setTitle(title);
            m_appliedTitle = std::move(title);
        }
        if (m_client.failed()) {
            requestStop();
        }
    }

    void EditorApp::wakeMainThread() {
        m_client.wakeMain();
    }

    bool EditorApp::renderTiming(RenderTiming& out) const {
        return m_client.renderTiming(out);
    }

    void EditorApp::shutdown() {
        m_client.stop();
        if (m_client.failed()) {
            throw std::runtime_error{"Render thread failed"};
        }
    }

    bool EditorApp::shouldClose() const {
        return m_client.shouldClose();
    }

    Role EditorApp::editorRole() {
        return Role::Client;
    }
}
