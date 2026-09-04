#include "EditorApp.hpp"

#include <TechEngine/platform/Platform.hpp>

#include "TechEngine/base/diagnostics/Assert.hpp"
#include "TechEngine/base/diagnostics/Log.hpp"

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
    }

    void EditorApp::fixedUpdate(const FrameContext& frame) {
        TE_LOGGER_INFO("Editor fixedUpdate: tick {0}, frame {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.frameIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
    }

    void EditorApp::update(const FrameContext& frame) {
        TE_LOGGER_INFO("Editor update: tick {0}, frame {1}, deltaTime {2}, fixedDeltaTime {3}, alpha {4}, role {5}", frame.tick, frame.frameIndex, frame.deltaTime, frame.fixedDeltaTime, frame.alpha, toString(frame.role));
    }

    void EditorApp::shutdown() {
    }

    Role EditorApp::editorRole() {
        return Role::Client;
    }
}
