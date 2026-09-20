#include <RuntimeApp.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>

namespace {
    class RuntimeProbe : public TechEngine::RuntimeApp {
    public:
        void bootstrapSimulation() {
            finalizeSimulation();
        }

        void advanceFrame() {
            m_simulationThread.advance(TechEngine::SimulationSettings::FIXED_DELTA_TIME, [this](const TechEngine::SimulationContext& simulation) {
                executeSimulationTick(simulation);
            });
        }

        std::size_t entityCount() const {
            return m_scene.getRoots().size();
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }
    };
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("runtime executes its configured demo systems through App", "[runtime]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();

    CHECK(runtime.entityCount() == 0);
    runtime.advanceFrame();
    CHECK(runtime.entityCount() == 1);
}
