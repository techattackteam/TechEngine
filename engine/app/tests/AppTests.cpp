#include <TechEngine/app/App.hpp>
#include <TechEngine/platform/files/FileResult.hpp>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

namespace {
    class ProbeApp : public TechEngine::App {
    public:
        explicit ProbeApp(TechEngine::Role role) : App(role) {
        }

        void init() override {
        }

        void fixedUpdate(const TechEngine::FrameContext&) override {
        }

        void update(const TechEngine::FrameContext&) override {
        }

        void shutdown() override {
        }

        TechEngine::Role loopRole() const {
            return m_loop.frame().role;
        }

        TechEngine::MountTable& table() {
            return m_mounts;
        }

        const TechEngine::EngineContext& context() const {
            return m_engine;
        }
    };
}

TEST_CASE("the subclass's role reaches the loop App owns", "[app]") {
    const ProbeApp app{TechEngine::Role::DedicatedServer};

    REQUIRE(app.loopRole() == TechEngine::Role::DedicatedServer);
}

TEST_CASE("a mount added after construction is visible through App's context", "[app]") {
    ProbeApp app{TechEngine::Role::Client};

    app.table().mount("scratch", std::filesystem::temp_directory_path());

    std::filesystem::path resolved;
    const TechEngine::FileResult result = app.context().files.resolve("scratch://no-such-file", resolved);

    // NotFound means the context saw the mount and the file was absent, which is the point.
    // NoMount would mean EngineContext had snapshotted the table instead of referencing it,
    // and init() could then never mount anything the loop reads.
    REQUIRE(result == TechEngine::FileResult::NotFound);
}
