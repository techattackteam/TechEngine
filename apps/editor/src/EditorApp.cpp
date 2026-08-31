#include "EditorApp.hpp"

#include "TechEngine/base/diagnostics/Log.hpp"

namespace TechEngine {

    EditorApp::EditorApp() : App(editorRole()) {
    }

    EditorApp::~EditorApp() {
    }

    void EditorApp::init() {
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
