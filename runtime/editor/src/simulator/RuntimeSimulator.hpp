#pragma once
#include "events/application/AppCloseEvent.hpp"
#include "events/scripts/ScriptCrashEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "project/Project.hpp"
#include "renderer/Renderer.hpp"
#include "systems/System.hpp"
#include "physics/PhysicsEngine.hpp"
#include "core/timer.hpp"
#include <Jolt/Jolt.h>

#include "renderer/OldRenderer.hpp"

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
        SystemsRegistry& m_systemsRegistry;
        bool m_stopNextUpdate = false;

    public:
        T& m_runtime;

        explicit RuntimeSimulator(T& runtime, SystemsRegistry& m_systemRegistry) : m_runtime(runtime), m_systemsRegistry(m_systemRegistry) {
        }

        bool startSimulation(const std::string& dllPath, const std::shared_ptr<ImGuiSink<std::mutex>>& sink) {
            ScriptEngine& scriptEngine = m_runtime.m_systemRegistry.template getSystem<ScriptEngine>();
            PhysicsEngine& physicsEngine = m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>();
            ScenesManager& scenesManager = m_runtime.m_systemRegistry.template getSystem<ScenesManager>();
            Project& project = m_runtime.m_systemRegistry.template getSystem<Project>();
            scenesManager.copyScene(scenesManager.getActiveScene(), project.getPath(PathType::Cache, AppType::Client) / "runtimeScene.tescene");
            spdlog::sinks::dist_sink_mt* userDllSink;
            bool result;
            std::tie(result, userDllSink) = scriptEngine.start(dllPath);
            if (!result) {
                TE_LOGGER_ERROR("Failed to load client scripts dll");
                return false;
            }
            if (!physicsEngine.start()) {
                return false;
            }
            userDllSink->add_sink(sink);
            m_simulationState = SimulationState::RUNNING;
            onStart();
            return true;
        }

        void pauseSimulation() {
            m_simulationState = SimulationState::PAUSED;
        }

        void stopSimulation() {
            onStop();
            m_simulationState = SimulationState::STOPPED;
            ScenesManager& scenesManager = m_runtime.m_systemRegistry.template getSystem<ScenesManager>();
            Project& project = m_runtime.m_systemRegistry.template getSystem<Project>();
            scenesManager.loadScene(project.getPath(PathType::Cache, AppType::Client) / "runtimeScene.tescene");
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
            m_systemsRegistry.getSystem<EventDispatcher>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    stopSimulation();
                }
            });
            m_runtime.m_systemRegistry.template getSystem<EventDispatcher>().template subscribe<ScriptCrashEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    m_stopNextUpdate = true;
                }
            });
            //Renderer& renderer = m_runtime.m_systemRegistry.template getSystem<Renderer>();
            //DebugRenderer* debugRenderer = m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>().debugRenderer;
            //debugRenderer->init([this](const glm::vec3& from, const glm::vec3& to, const glm::vec4& color) {
            //    this->m_runtime.m_systemRegistry.template getSystem<Renderer>().createLine(from, to, color);
            //});
            //m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>().debugRenderer = debugRenderer;
            JPH::RegisterDefaultAllocator();
        }

        void onStart() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onStart();
            }
        }

        void onFixedUpdate() override {
            //if (m_simulationState == SimulationState::RUNNING) {
            //    m_runtime.onFixedUpdate();
            //}
        }

        void onUpdate() override {
            //if (m_simulationState == SimulationState::RUNNING) {
            //    m_runtime.onUpdate();
            //}
        }

        void onStop() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onStop();
            }
        }

        void shutdown() override {
            m_runtime.shutdown();
        }

        void tick(float deltaTime) {
            if (m_simulationState != SimulationState::RUNNING) {
                //m_runtime.m_systemRegistry.template getSystem<EventDispatcher>().onUpdate();
            } else {
                Timer& timer = m_runtime.m_systemRegistry.template getSystem<Timer>();
                timer.tick(deltaTime);
                m_runtime.onUpdate();
                timer.addAccumulator(deltaTime);
                while (timer.getAccumulator() >= timer.getTPS()) {
                    timer.updateTicks();
                    m_runtime.onFixedUpdate();
                    timer.addAccumulator(-timer.getTPS());
                }
                timer.updateInterpolation();
                if (m_stopNextUpdate) {
                    stopSimulation();
                    m_stopNextUpdate = false;
                }
            }
        }

        const SimulationState& getSimulationState() const {
            return m_simulationState;
        }
    };
}
