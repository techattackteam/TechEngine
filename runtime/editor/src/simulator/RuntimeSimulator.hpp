#pragma once
#include "events/application/AppCloseEvent.hpp"
#include "events/scripts/ScriptCrashEvent.hpp"
#include "eventSystem/EventManager.hpp"
#include "../project/Project.hpp"
#include "renderer/Renderer.hpp"
#include "systems/System.hpp"
#include "physics/PhysicsEngine.hpp"
#include "core/timer.hpp"
#include "logger/ImGuiSink.hpp"
#include "script/ScriptEngine.hpp"
#include "scene/SceneManager.hpp"
#include "resources/loaders/SceneLoader.hpp"
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
        SystemsRegistry& m_systemsRegistry;
        bool m_stopNextUpdate = false;
        std::string m_preSimulationSceneName;
        SceneLoader* m_sceneLoader = nullptr;

    public:
        T& m_runtime;

        explicit RuntimeSimulator(T& runtime,
                                  SystemsRegistry& m_systemRegistry) : m_runtime(runtime),
                                                                       m_systemsRegistry(m_systemRegistry) {
        }

        void setSceneLoader(SceneLoader* sceneLoader) {
            m_sceneLoader = sceneLoader;
        }

        bool startSimulation(const std::string& dllPath, const std::shared_ptr<ImGuiSink<std::mutex>>& sink) {
            ScriptEngine& scriptEngine = m_runtime.m_systemRegistry.template getSystem<ScriptEngine>();
            PhysicsEngine& physicsEngine = m_runtime.m_systemRegistry.template getSystem<PhysicsEngine>();
            SceneManager& sceneManager = m_runtime.m_systemRegistry.template getSystem<SceneManager>();

            m_preSimulationSceneName = sceneManager.getActiveSceneName();
            if (m_sceneLoader) {
                m_sceneLoader->saveActiveScene();
            }

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
            if (m_sceneLoader) {
                m_sceneLoader->loadScene(m_preSimulationSceneName);
            }
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
            m_systemsRegistry.getSystem<EventManager>().subscribe<AppCloseEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    stopSimulation();
                }
            });
            m_runtime.m_systemRegistry.template getSystem<EventManager>().template subscribe<ScriptCrashEvent>([this](const std::shared_ptr<Event>& event) {
                if (m_simulationState != SimulationState::STOPPED) {
                    m_stopNextUpdate = true;
                }
            });
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
                m_runtime.m_systemRegistry.template getSystem<EventManager>().onUpdate();
                m_runtime.m_systemRegistry.template getSystem<Renderer>().onUpdate();
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
