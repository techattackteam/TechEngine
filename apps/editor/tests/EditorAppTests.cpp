#include "EditorApp.hpp"

#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string_view>

using TechEngineTests::ScratchDirectory;

namespace {
    class EditorProbe : public TechEngine::EditorApp {
    public:
        using TechEngine::EditorApp::EditorApp;
        using TechEngine::EditorApp::shouldClose;
        using TechEngine::EditorApp::shutdown;

        void advanceFrame(double deltaTime) {
            m_loop.advance(deltaTime, [this](const TechEngine::FrameContext& frame) {
                fixedUpdate(frame);
            });
            update(m_loop.frame());
        }

        bool ratesUpdated() const {
            return m_loop.ratesUpdated();
        }

        TechEngine::Role loopRole() const {
            return m_loop.frame().role;
        }

        void bootstrap() {
            init();
        }

        const TechEngine::MountTable& mounts() const {
            return m_mounts;
        }

        const TechEngine::FileAccess& files() const {
            return m_files;
        }
    };

    constexpr std::string_view DEV_MANIFEST = "name = \"Dev\"\n";

    void writeProjectLayout(const ScratchDirectory& scratch) {
        scratch.writeFile("project.toml", DEV_MANIFEST);
        scratch.makeDirectory("assets/common");
        scratch.makeDirectory("assets/client");
        scratch.makeDirectory("assets/server");
        scratch.makeDirectory("shaders");
    }
}

TEST_CASE("editor hosts a client", "[editor]") {
    const EditorProbe editor{std::filesystem::path{}};

    REQUIRE(editor.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("init mounts the project set", "[editor]") {
    ScratchDirectory scratch{"editorBootstrap"};
    writeProjectLayout(scratch);

    EditorProbe editor{scratch.root()};
    editor.bootstrap();

    CHECK(editor.mounts().hasAlias("project"));
    CHECK(editor.mounts().hasAlias("engine"));
    CHECK(editor.mounts().hasAlias("shaders"));
    CHECK(editor.mounts().hasAlias("assets"));
    CHECK(editor.mounts().mountCount() == 5);
}

TEST_CASE("assets/client shadows assets/common", "[editor]") {
    ScratchDirectory scratch{"editorAssetsOverlay"};
    writeProjectLayout(scratch);
    scratch.writeFile("assets/common/shared.txt", "common");
    const std::filesystem::path client = scratch.writeFile("assets/client/shared.txt", "client");

    EditorProbe editor{scratch.root()};
    editor.bootstrap();

    std::filesystem::path resolved;
    REQUIRE(editor.files().resolve("assets://shared.txt", resolved) == TechEngine::FileResult::Ok);
    CHECK(resolved == client);
}

TEST_CASE("editor updates through a rate sample and shuts down its client", "[editor][window]") {
    ScratchDirectory scratch{"editorLifecycle"};
    writeProjectLayout(scratch);
    EditorProbe editor{scratch.root()};
    CHECK(editor.shouldClose());
    editor.bootstrap();
    REQUIRE_FALSE(editor.shouldClose());

    for (int i = 0; i < 8; i++) {
        editor.advanceFrame(0.125);
        CHECK(editor.ratesUpdated() == (i == 7));
        CHECK_FALSE(editor.shouldClose());
    }

    editor.shutdown();
    CHECK(editor.shouldClose());
    editor.shutdown();
}
