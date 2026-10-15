#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/client/render/RenderSnapshot.hpp>
#include <TechEngine/core/assets/PackSource.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>

#include <RuntimeApp.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view ASSETS_DIRECTORY = "assets://";
    static constexpr std::string_view COMMON_PACK_PATH = "assets://common.tepack";

    RuntimeApp::RuntimeApp(std::filesystem::path folder) : App(runtimeRole()), m_folder(std::move(folder)) {
    }

    void RuntimeApp::init() {
        mountAssets();
        const std::error_code startError = m_client.start(m_engine, m_input, 1280, 720, "TechEngine Runtime Demo", [this] {
            requestStop();
        });
        if (startError) {
            throw std::runtime_error{"Failed to start the client session: " + startError.message()};
        }
    }

    void RuntimeApp::mountAssets() {
        m_mounts.mount("assets", m_folder / "assets");
    }

    std::error_code RuntimeApp::simulationStartupLoad() {
        PackReader common(m_files);
        const std::error_code commonError = common.open(COMMON_PACK_PATH);
        if (commonError) {
            TE_LOGGER_ERROR("Failed to open {0}: {1}", COMMON_PACK_PATH, commonError.message());
            return commonError;
        }

        PackEntry bootEntry;
        if (!common.bootRecord(bootEntry)) {
            TE_LOGGER_ERROR("{0} holds no boot record", COMMON_PACK_PATH);
            return ReadError::Malformed;
        }
        std::vector<std::byte> bootBytes;
        const std::error_code bootReadError = common.read(bootEntry, bootBytes);
        if (bootReadError) {
            return bootReadError;
        }
        BootRecord record;
        const std::error_code bootParseError = record.read(bootBytes);
        if (bootParseError) {
            TE_LOGGER_ERROR("The boot record in {0} does not parse: {1}", COMMON_PACK_PATH, bootParseError.message());
            return bootParseError;
        }
        if (!record.startupScene.valid()) {
            TE_LOGGER_ERROR("The boot record in {0} names no startup scene", COMMON_PACK_PATH);
            return ReadError::Malformed;
        }

        PackSource source(m_files);
        const std::error_code sourceError = source.open(record, runtimeRole(), ASSETS_DIRECTORY);
        if (sourceError) {
            TE_LOGGER_ERROR("Failed to open the client's packs under {0}: {1}", ASSETS_DIRECTORY, sourceError.message());
            return sourceError;
        }
        if (source.find(record.startupScene) != AssetAvailability::Present) {
            TE_LOGGER_ERROR("The client's packs do not hold the startup scene {0:#018x}", record.startupScene.value());
            return ReadError::Malformed;
        }

        std::vector<std::byte> sceneBytes;
        const std::error_code sceneReadError = source.read(record.startupScene, sceneBytes);
        if (sceneReadError) {
            return sceneReadError;
        }
        SceneAsset scene;
        const std::error_code sceneParseError = scene.read(sceneBytes, m_scene);
        if (sceneParseError) {
            TE_LOGGER_ERROR("The startup scene {0:#018x} does not parse: {1}", record.startupScene.value(), sceneParseError.message());
            return sceneParseError;
        }
        const std::vector<Entity> roots = m_scene.getRoots();
        TE_LOGGER_INFO("Loaded the startup scene {0:#018x} with {1} root entities", record.startupScene.value(), roots.size());
        for (const Entity root: roots) {
            logSceneTree(root, 1);
        }
        return {};
    }

    void RuntimeApp::logSceneTree(const Entity entity, const std::size_t depth) const {
        const Vec3& position = m_scene.getComponent<Transform>(entity).getLocal().position;
        TE_LOGGER_INFO("{0}Entity {1}:{2} at ({3}, {4}, {5})", std::string(depth * 2, ' '), entity.index, entity.generation, position.x, position.y, position.z);
        for (const Entity child: m_scene.getChildren(entity)) {
            logSceneTree(child, depth + 1);
        }
    }

    void RuntimeApp::publishSnapshot(const SimulationContext& simulation) {
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

    void RuntimeApp::mainThreadUpdate() {
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
        std::string title = "TechEngine Runtime Demo | FPS: " + std::to_string(fps) + " | TPS: " + std::to_string(tps);
        if (title != m_appliedTitle) {
            m_client.setTitle(title);
            m_appliedTitle = std::move(title);
        }
        if (m_client.failed()) {
            requestStop();
        }
    }

    void RuntimeApp::wakeMainThread() {
        m_client.wakeMain();
    }

    bool RuntimeApp::renderTiming(RenderTiming& out) const {
        return m_client.renderTiming(out);
    }

    void RuntimeApp::shutdown() {
        m_client.stop();
        if (m_client.failed()) {
            throw std::runtime_error{"Render thread failed"};
        }
    }

    bool RuntimeApp::shouldClose() const {
        return m_client.shouldClose();
    }

    Role RuntimeApp::runtimeRole() {
        return Role::Client;
    }
}
