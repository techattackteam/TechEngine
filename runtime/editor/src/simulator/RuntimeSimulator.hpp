#pragma once
#include "events/application/AppCloseEvent.hpp"
#include "events/scripts/ScriptCrashEvent.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "renderer/Renderer.hpp"
#include "systems/System.hpp"


namespace TechEngine {
    enum class SimulationState {
        STOPPED,
        RUNNING,
        PAUSED
    };

    template<typename T>
    class RuntimeSimulator : public System {
    private:
        T& m_runtime;
        T m_runtimeCopy;
        SimulationState m_simulationState = SimulationState::STOPPED;
        SystemsRegistry& m_systemRegistry;

    public:
        explicit RuntimeSimulator(T& runtime, SystemsRegistry& m_systemRegistry) : m_runtime(runtime), m_runtimeCopy(runtime), m_systemRegistry(m_systemRegistry) {
        }

        void startSimulation() {
            m_simulationState = SimulationState::RUNNING;
            //Create a deep copy of the runtime
            m_runtimeCopy = m_runtime;
            onStart();
        }

        void pauseSimulation() {
            m_simulationState = SimulationState::PAUSED;
        }

        void stopSimulation() {
            m_simulationState = SimulationState::STOPPED;
            onStop();
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
                    stopSimulation();
                }
            });
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
        }

        void onStop() override {
            if (m_simulationState == SimulationState::RUNNING) {
                m_runtime.onStop();
            }
        }

        void shutdown() override {
            /*if (m_simulationState == SimulationState::RUNNING) {
            }*/
            m_runtime.shutdown();
        }

        const SimulationState& getSimulationState() const {
            return m_simulationState;
        }
    };
}
