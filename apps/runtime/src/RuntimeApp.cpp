#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/client/render/RenderSnapshot.hpp>

#include <RuntimeApp.hpp>
#include <demo/CollisionSystem.hpp>
#include <demo/EntitySpawnSystem.hpp>
#include <demo/GravitySystem.hpp>
#include <demo/MovementSystem.hpp>
#include <demo/RigidBody.hpp>
#include <demo/Velocity.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace TechEngine {
    RuntimeApp::RuntimeApp() : App(runtimeRole()) {
    }

    void RuntimeApp::init() {
        if (!m_client.start(m_engine, m_input, 1280, 720, "TechEngine Runtime Demo", [this] {
                requestStop();
            })) {
            throw std::runtime_error{"Failed to start the client session"};
        }
    }

    void RuntimeApp::configureSimulation() {
        m_registry.registerComponent<Velocity>(Velocity::tag);
        m_registry.registerComponent<RigidBody>(RigidBody::tag);
        m_schedule.add<MovementSystem>();
        m_schedule.add<GravitySystem>();
        m_schedule.add<CollisionSystem>();
        m_schedule.add<EntitySpawnSystem>();
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
        const auto fps = static_cast<std::uint64_t>(metrics.render ? metrics.render->framesPerSecond : 0.0);
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

    std::optional<RenderTiming> RuntimeApp::renderTiming() const {
        return m_client.renderTiming();
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
