#include "EditorApp.hpp"

#include <TechEngine/testing/ScratchDirectory.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <thread>

using TechEngineTests::ScratchDirectory;

namespace {
    class EditorProbe : public TechEngine::EditorApp {
    public:
        using TechEngine::EditorApp::EditorApp;
        using TechEngine::EditorApp::shouldClose;
        using TechEngine::EditorApp::shutdown;

        void advanceFrame(double deltaTime) {
            m_simulationThread.advance(deltaTime, [this](const TechEngine::SimulationContext& frame) {
                fixedUpdate(frame);
            });
            publishSnapshot(m_simulationThread.simulationContext());
        }

        double ticksPerSecond() const {
            return m_simulationThread.timing().ticksPerSecond;
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
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
        CHECK(editor.ticksPerSecond() == (i == 7 ? 60.0 : 0.0));
        CHECK_FALSE(editor.shouldClose());
    }

    editor.shutdown();
    CHECK(editor.shouldClose());
    editor.shutdown();
}

class EditorStallProbe : public TechEngine::EditorApp {
public:
    using EditorApp::EditorApp;
    std::atomic<bool> mainEntered = false;
    std::atomic<bool> releaseMain = false;
    std::atomic<bool> inputConsumed = false;
    std::atomic<bool> mainTimedOut = false;

protected:
    void mainUpdate() override {
        mainEntered.store(true);
        const auto deadline = m_clock.now() + std::chrono::seconds{5};
        while (!releaseMain.load() && m_clock.now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        if (!releaseMain.load()) {
            mainTimedOut = true;
            requestStop();
            return;
        }
        m_input.publish(TechEngine::InputEvent{.kind = TechEngine::InputKind::Focus, .pressed = true});
        m_input.publish(TechEngine::InputEvent{.kind = TechEngine::InputKind::Key, .code = 87, .pressed = true});
        EditorApp::mainUpdate();
    }
    void fixedUpdate(const TechEngine::SimulationContext& simulation) override {
        if (simulation.input.held.keys.test(87)) {
            inputConsumed = true;
            requestStop();
        }
    }
};

TEST_CASE("editor simulation and render progress during a main stall then consume delayed input", "[editor][window][integration]") {
    ScratchDirectory scratch{"editorThreadedStall"};
    writeProjectLayout(scratch);
    EditorStallProbe editor{scratch.root()};
    std::atomic<bool> progressed = false;
    std::jthread controller{[&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (!editor.mainEntered.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        const auto before = editor.timingMetrics();
        if (editor.mainEntered.load() && before.render) {
            while (std::chrono::steady_clock::now() < deadline) {
                const auto after = editor.timingMetrics();
                if (after.render && after.simulation.tick >= before.simulation.tick + 3 && after.render->frame >= before.render->frame + 3) {
                    progressed = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        }
        editor.releaseMain = true;
        if (!progressed.load()) {
            editor.requestStop();
        }
    }};
    const int result = editor.run();
    controller.join();
    CHECK(result == 0);
    CHECK(progressed.load());
    CHECK(editor.inputConsumed.load());
    CHECK_FALSE(editor.mainTimedOut.load());
    CHECK_FALSE(editor.timingMetrics().render);
}
