#pragma once
#include "events/application/AppCloseEvent.hpp"
#include "events/scripts/ScriptCrashEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "project/Project.hpp"
#include "renderer/Renderer.hpp"
#include "systems/System.hpp"
#include "physics/PhysicsEngine.hpp"
#include <Jolt/Jolt.h>

namespace TechEngine {
    enum class SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };

    template<typename T>
    class RuntimeSimulator : public System {
    private:
        SimulationState m_simulationState = SimulationState::STOPPED;
        SystemsRegistry& m_systemRegistry;
        bool stopNextUpdate = false;

    public:
        T& m_runtime;

        explicit RuntimeSimulator(T& runtime, SystemsRegistry& m_systemRegistry) : m_runtime(runtime), m_systemRegistry(m_systemRegistry) {
        }

        void startSimulation() {
            m_simulationState = SimulationState::RUNNING;
            ScenesManager& scenesManager = m_runtime.m_systemRegistry.template getSystem<ScenesManager>();
            Project& project = m_runtime.m_systemRegistry.template getSystem<Project>();
            scenesManager.copyScene(scenesManager.getActiveScene(), project.getPath(PathType::Cache, AppType::Client) / "runtimeScene.tescene");
            onStart();
        }

        void pauseSimulation() {
            m_simulationState = SimulationState::PAUSED;
        }

        void stopSimulation() {
            m_simulationState = SimulationState::STOPPED;
            onStop();
            ScenesManager& scenesManager = m_runtime.m_systemRegistry.template getSystem<ScenesManager>();
            scenesManager.loadScene(scenesManager.getActiveScene().getName());
            shutdown();
            init();
        }

        void registerSystems(const std::filesystem::path& rootPath) {
            m_runtime.registerSystems(rootPath);
        }

        void init() override {
            m_runtime.init();
            if (!m_runtime.m_systemRegistry.template hasSystem<Renderer>()) {
                m_runtime.m_systemRegistry.template registerSystem<Renderer>(m_runtime.m_systemRegistry);
                m_runtime.m_systemRegistry.template getSystem<Renderer>().init();
            }
            m_systemRegistry.getSystem<EventDispatcher>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    stopSimulation();
                }
            });
            m_runtime.m_systemRegistry.getSystem<EventDispatcher>().subscribe<ScriptCrashEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    stopNextUpdate = true;
                }
            });
            Renderer& renderer = m_runtime.m_systemRegistry.template getSystem<Renderer>();
            DebugRenderer* debugRenderer = m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>().debugRenderer;
            debugRenderer->init([this](const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) {
                this->m_runtime.m_systemRegistry.template getSystem<Renderer>().createLine(from, to, color);
            });
            m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>().debugRenderer = debugRenderer;
        }

        void onStart() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onStart();
            }
        }

        void onFixedUpdate() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onFixedUpdate();
            }
        }

        void onUpdate() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onUpdate();
            }
            if (stopNextUpdate) {
                stopSimulation();
                stopNextUpdate = false;
            }
        }

        void onStop() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onStop();
            }
        }

        void shutdown() override {
            m_runtime.shutdown();
        }

        const SimulationState& getSimulationState() const {
            return m_simulationState;
        }
    };
}
