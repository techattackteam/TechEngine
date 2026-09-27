#include <TechEngine/core/systems/ISystem.hpp>

#include <RuntimeApp.hpp>
#include <demo/CollisionSystem.hpp>
#include <demo/EntitySpawnSystem.hpp>
#include <demo/GravitySystem.hpp>
#include <demo/MovementSystem.hpp>
#include <demo/RigidBody.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <typeinfo>
#include <vector>

namespace {
    class RuntimeProbe : public TechEngine::RuntimeApp {
    public:
        void bootstrapSimulation() {
            finalizeSimulation();
        }

        std::uint64_t advanceSteps(double fixedSteps) {
            const std::uint64_t before = m_simulationThread.simulationContext().tick;
            m_simulationThread.advance(fixedSteps * TechEngine::SimulationSettings::FIXED_DELTA_TIME, [this](const TechEngine::SimulationContext& simulation) {
                executeSimulationTick(simulation);
            });
            return m_simulationThread.simulationContext().tick - before;
        }

        std::size_t entityCount() const {
            return m_scene.getRoots().size();
        }

        std::vector<TechEngine::Entity> entities() const {
            return m_scene.getRoots();
        }

        bool contains(const TechEngine::Entity entity) const {
            return m_scene.contains(entity);
        }

        std::size_t groundedCount() {
            std::size_t grounded = 0;
            m_scene.query<TechEngine::Write<>, TechEngine::Read<TechEngine::RigidBody>>().each([&grounded](const TechEngine::Entity, const TechEngine::RigidBody& rigidBody) {
                if (rigidBody.grounded) {
                    grounded++;
                }
            });
            return grounded;
        }

        template<std::derived_from<TechEngine::ISystem> System>
        const System& system() const {
            const std::size_t index = m_schedule.getEntryByType().at(typeid(System));
            return static_cast<const System&>(*m_schedule.getEntries()[index].system);
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }
    };
}

using Batches = std::vector<std::size_t>;

static void runUntilFirstLanding(RuntimeProbe& runtime) {
    runtime.bootstrapSimulation();
    // Half a step keeps the accumulator off the fixed-step boundary, where float drift would decide the tick count.
    REQUIRE(runtime.advanceSteps(0.5) == 0);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.groundedCount() == 1);
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("runtime executes its configured demo systems through App", "[runtime]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();

    CHECK(runtime.entityCount() == 0);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    CHECK(runtime.entityCount() == 1);
}

TEST_CASE("a demo landing reaches no handler in the Tick that publishes it", "[runtime][events]") {
    RuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches().empty());
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches().empty());
}

TEST_CASE("a demo landing reaches every Landed handler, regular and terminal, in the next Tick", "[runtime][events]") {
    RuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches() == Batches{1});
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches() == Batches{1});
}

TEST_CASE("a landed entity survives the Tick it lands in and is destroyed at the next Tick's barrier", "[runtime][events]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();
    REQUIRE(runtime.advanceSteps(0.5) == 0);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.entities().size() == 1);
    const TechEngine::Entity first = runtime.entities().front();

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.contains(first));

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK_FALSE(runtime.contains(first));
    CHECK(runtime.entityCount() == 2);
}

TEST_CASE("a zero-Tick advance neither delivers nor retires a visible demo landing", "[runtime][events]") {
    RuntimeProbe runtime;
    runUntilFirstLanding(runtime);
    const std::size_t entitiesBefore = runtime.entityCount();

    REQUIRE(runtime.advanceSteps(0.25) == 0);

    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches().empty());
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches().empty());
    CHECK(runtime.entityCount() == entitiesBefore);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches() == Batches{1});
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches() == Batches{1});
}

TEST_CASE("a multi-Tick catch-up delivers each Tick's landing separately in the Tick after it", "[runtime][events]") {
    RuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    REQUIRE(runtime.advanceSteps(3.0) == 3);

    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches() == Batches{1, 1, 1});
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches() == Batches{1, 1, 1});
}

TEST_CASE("every demo landing reaches each handler exactly once and quiet Ticks call no handler", "[runtime][events]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();
    REQUIRE(runtime.advanceSteps(0.5) == 0);

    for (int i = 0; i < 110; i++) {
        REQUIRE(runtime.advanceSteps(1.0) == 1);
    }

    CHECK(runtime.entityCount() == 0);
    CHECK(runtime.system<TechEngine::GravitySystem>().getLandedBatches() == Batches(100, 1));
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getLandedBatches() == Batches(100, 1));

    std::vector<std::uint32_t> ordinals(100);
    std::iota(ordinals.begin(), ordinals.end(), 0U);
    CHECK(runtime.system<TechEngine::CollisionSystem>().getSpawnOrdinals() == ordinals);
    CHECK(runtime.system<TechEngine::GravitySystem>().getDespawned().size() == 100);
    CHECK(runtime.system<TechEngine::MovementSystem>().getDespawned() == runtime.system<TechEngine::GravitySystem>().getDespawned());
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getPulseBatches() == Batches(109, TechEngine::MovementSystem::PULSES_PER_TICK));
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().isPulseSequenceIntact());
}

TEST_CASE("a spawn published from the terminal slot reaches a regular-slot handler in the next Tick", "[runtime][events]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();
    REQUIRE(runtime.advanceSteps(0.5) == 0);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::CollisionSystem>().getSpawnOrdinals().empty());

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::CollisionSystem>().getSpawnOrdinals() == std::vector<std::uint32_t>{0});

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::CollisionSystem>().getSpawnOrdinals() == std::vector<std::uint32_t>{0, 1});
}

TEST_CASE("a despawn published by a handler reaches every handler of its type one Tick later", "[runtime][events]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();
    REQUIRE(runtime.advanceSteps(0.5) == 0);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.entities().size() == 1);
    const TechEngine::Entity first = runtime.entities().front();

    REQUIRE(runtime.advanceSteps(2.0) == 2);

    CHECK(runtime.system<TechEngine::GravitySystem>().getDespawned().empty());
    CHECK(runtime.system<TechEngine::MovementSystem>().getDespawned().empty());

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<TechEngine::GravitySystem>().getDespawned() == std::vector<TechEngine::Entity>{first});
    CHECK(runtime.system<TechEngine::MovementSystem>().getDespawned() == std::vector<TechEngine::Entity>{first});
}

TEST_CASE("a burst of pulses every Tick arrives whole and in order, including across a catch-up", "[runtime][events]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();
    REQUIRE(runtime.advanceSteps(0.5) == 0);

    for (int i = 0; i < 5; i++) {
        REQUIRE(runtime.advanceSteps(1.0) == 1);
    }
    REQUIRE(runtime.advanceSteps(5.0) == 5);

    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().getPulseBatches() == Batches(9, TechEngine::MovementSystem::PULSES_PER_TICK));
    CHECK(runtime.system<TechEngine::EntitySpawnSystem>().isPulseSequenceIntact());
}
