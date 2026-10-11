#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Query.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/ScheduleRegistration.hpp>

#include <RuntimeApp.hpp>

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <string_view>
#include <typeinfo>
#include <vector>

namespace {
    struct FallingBody {
        static constexpr std::string_view tag = "Test.RuntimeFallingBody";

        bool grounded = false;
    };

    struct ProbeSpawned {
        static constexpr std::string_view tag = "Test.RuntimeSpawned";

        std::uint32_t ordinal;
    };

    struct ProbeLanded {
        static constexpr std::string_view tag = "Test.RuntimeLanded";

        TechEngine::Entity entity;
    };

    struct ProbeDespawned {
        static constexpr std::string_view tag = "Test.RuntimeDespawned";

        TechEngine::Entity entity;
    };

    struct ProbePulse {
        static constexpr std::string_view tag = "Test.RuntimePulse";

        std::uint32_t sequence;
    };

    class PulseSystem final : public TechEngine::ISystem {
    private:
        std::uint32_t m_nextPulse = 0;
        std::vector<TechEngine::Entity> m_despawned;

    public:
        static constexpr std::uint32_t PULSES_PER_TICK = 500;

        void init(TechEngine::ScheduleRegistration& registration) override {
            registration.onEvent<ProbeDespawned>([this](TechEngine::Scene&, const std::span<const ProbeDespawned> despawns) {
                for (const ProbeDespawned& despawn: despawns) {
                    m_despawned.push_back(despawn.entity);
                }
            });
        }

        void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
            for (std::uint32_t i = 0; i < PULSES_PER_TICK; i++) {
                scene.publish<ProbePulse>(m_nextPulse++);
            }
        }

        std::string_view name() const override {
            return "PulseSystem";
        }

        const std::vector<TechEngine::Entity>& getDespawned() const {
            return m_despawned;
        }
    };

    class LandingObserver final : public TechEngine::ISystem {
    private:
        std::vector<std::size_t> m_landedBatches;
        std::vector<TechEngine::Entity> m_despawned;

    public:
        void init(TechEngine::ScheduleRegistration& registration) override {
            registration.onEvent<ProbeLanded>([this](TechEngine::Scene&, const std::span<const ProbeLanded> landings) {
                m_landedBatches.push_back(landings.size());
            });
            registration.onEvent<ProbeDespawned>([this](TechEngine::Scene&, const std::span<const ProbeDespawned> despawns) {
                for (const ProbeDespawned& despawn: despawns) {
                    m_despawned.push_back(despawn.entity);
                }
            });
        }

        void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        }

        std::string_view name() const override {
            return "LandingObserver";
        }

        const std::vector<std::size_t>& getLandedBatches() const {
            return m_landedBatches;
        }

        const std::vector<TechEngine::Entity>& getDespawned() const {
            return m_despawned;
        }
    };

    class LandingSystem final : public TechEngine::ISystem {
    private:
        std::vector<std::uint32_t> m_spawnOrdinals;

    public:
        void init(TechEngine::ScheduleRegistration& registration) override {
            registration.access(TechEngine::DeclareAccess<TechEngine::Write<FallingBody>, TechEngine::Read<>>{});
            registration.onEvent<ProbeSpawned>([this](TechEngine::Scene&, const std::span<const ProbeSpawned> spawns) {
                for (const ProbeSpawned& spawn: spawns) {
                    m_spawnOrdinals.push_back(spawn.ordinal);
                }
            });
        }

        void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
            scene.query<TechEngine::Write<FallingBody>, TechEngine::Read<>>().each([&scene](const TechEngine::Entity entity, FallingBody& body) {
                if (!body.grounded) {
                    body.grounded = true;
                    scene.publish<ProbeLanded>(entity);
                }
            });
        }

        std::string_view name() const override {
            return "LandingSystem";
        }

        const std::vector<std::uint32_t>& getSpawnOrdinals() const {
            return m_spawnOrdinals;
        }
    };

    class SpawnSystem final : public TechEngine::ISystem {
    private:
        static constexpr std::uint32_t TARGET_ENTITY_COUNT = 100;
        std::uint32_t m_entityCount = 0;
        std::vector<std::size_t> m_landedBatches;
        std::vector<std::size_t> m_pulseBatches;
        std::uint32_t m_nextPulse = 0;
        bool m_pulseSequenceIntact = true;

    public:
        void init(TechEngine::ScheduleRegistration& registration) override {
            registration.setSlot(TechEngine::Slot::Terminal);
            registration.onEvent<ProbeLanded>([this](TechEngine::Scene& scene, const std::span<const ProbeLanded> landings) {
                m_landedBatches.push_back(landings.size());
                for (const ProbeLanded& landing: landings) {
                    scene.getCommands().despawn(landing.entity);
                    scene.publish<ProbeDespawned>(landing.entity);
                }
            });
            registration.onEvent<ProbePulse>([this](TechEngine::Scene&, const std::span<const ProbePulse> pulses) {
                m_pulseBatches.push_back(pulses.size());
                for (const ProbePulse& pulse: pulses) {
                    if (pulse.sequence != m_nextPulse) {
                        m_pulseSequenceIntact = false;
                    }
                    m_nextPulse = pulse.sequence + 1;
                }
            });
        }

        void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
            if (m_entityCount >= TARGET_ENTITY_COUNT) {
                return;
            }
            const TechEngine::PendingEntity entity = scene.getCommands().spawn();
            scene.addComponent<FallingBody>(entity);
            scene.publish<ProbeSpawned>(m_entityCount);
            m_entityCount++;
        }

        std::string_view name() const override {
            return "SpawnSystem";
        }

        const std::vector<std::size_t>& getLandedBatches() const {
            return m_landedBatches;
        }

        const std::vector<std::size_t>& getPulseBatches() const {
            return m_pulseBatches;
        }

        bool isPulseSequenceIntact() const {
            return m_pulseSequenceIntact;
        }
    };

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

        std::size_t systemCount() const {
            return m_schedule.getEntries().size();
        }

        std::size_t eventTypeCount() const {
            return m_eventRegistry.typeCount();
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }
    };

    class EventRuntimeProbe final : public RuntimeProbe {
    public:
        std::size_t groundedCount() {
            std::size_t grounded = 0;
            m_scene.query<TechEngine::Write<>, TechEngine::Read<FallingBody>>().each([&grounded](const TechEngine::Entity, const FallingBody& body) {
                if (body.grounded) {
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

    protected:
        void configureSimulation() override {
            RuntimeProbe::configureSimulation();
            m_registry.registerComponent<FallingBody>(FallingBody::tag);
            m_eventRegistry.registerEvent<ProbeSpawned>();
            m_eventRegistry.registerEvent<ProbeLanded>();
            m_eventRegistry.registerEvent<ProbeDespawned>();
            m_eventRegistry.registerEvent<ProbePulse>();
            m_schedule.add<PulseSystem>();
            m_schedule.add<LandingObserver>();
            m_schedule.add<LandingSystem>();
            m_schedule.add<SpawnSystem>();
        }
    };
}

using Batches = std::vector<std::size_t>;

static void bootstrapOffStepBoundary(EventRuntimeProbe& runtime) {
    runtime.bootstrapSimulation();
    // Half a step keeps the accumulator off the fixed-step boundary, where float drift would decide the tick count.
    REQUIRE(runtime.advanceSteps(0.5) == 0);
}

static TechEngine::Entity spawnFirstEntity(EventRuntimeProbe& runtime) {
    bootstrapOffStepBoundary(runtime);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.entities().size() == 1);
    return runtime.entities().front();
}

static void runUntilFirstLanding(EventRuntimeProbe& runtime) {
    bootstrapOffStepBoundary(runtime);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.advanceSteps(1.0) == 1);
    REQUIRE(runtime.groundedCount() == 1);
}

TEST_CASE("runtime composes as a client", "[runtime]") {
    const RuntimeProbe runtime;

    REQUIRE(runtime.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("runtime configures no systems and no event types of its own", "[runtime]") {
    RuntimeProbe runtime;
    runtime.bootstrapSimulation();

    CHECK(runtime.systemCount() == 0);
    CHECK(runtime.eventTypeCount() == 0);
    REQUIRE(runtime.advanceSteps(1.5) == 1);
    CHECK(runtime.entityCount() == 0);
}

TEST_CASE("a landing reaches no handler in the Tick that publishes it", "[runtime][events]") {
    EventRuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    CHECK(runtime.system<LandingObserver>().getLandedBatches().empty());
    CHECK(runtime.system<SpawnSystem>().getLandedBatches().empty());
}

TEST_CASE("a landing reaches every handler, regular and terminal, in the next Tick", "[runtime][events]") {
    EventRuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingObserver>().getLandedBatches() == Batches{1});
    CHECK(runtime.system<SpawnSystem>().getLandedBatches() == Batches{1});
}

TEST_CASE("a landed entity survives the Tick it lands in and is destroyed at the next Tick's barrier", "[runtime][events]") {
    EventRuntimeProbe runtime;
    const TechEngine::Entity first = spawnFirstEntity(runtime);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.contains(first));

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK_FALSE(runtime.contains(first));
    CHECK(runtime.entityCount() == 2);
}

TEST_CASE("a zero-Tick advance neither delivers nor retires a visible landing", "[runtime][events]") {
    EventRuntimeProbe runtime;
    runUntilFirstLanding(runtime);
    const std::size_t entitiesBefore = runtime.entityCount();

    REQUIRE(runtime.advanceSteps(0.25) == 0);

    CHECK(runtime.system<LandingObserver>().getLandedBatches().empty());
    CHECK(runtime.system<SpawnSystem>().getLandedBatches().empty());
    CHECK(runtime.entityCount() == entitiesBefore);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingObserver>().getLandedBatches() == Batches{1});
    CHECK(runtime.system<SpawnSystem>().getLandedBatches() == Batches{1});
}

TEST_CASE("a multi-Tick catch-up delivers each Tick's landing separately in the Tick after it", "[runtime][events]") {
    EventRuntimeProbe runtime;
    runUntilFirstLanding(runtime);

    REQUIRE(runtime.advanceSteps(3.0) == 3);

    CHECK(runtime.system<LandingObserver>().getLandedBatches() == Batches{1, 1, 1});
    CHECK(runtime.system<SpawnSystem>().getLandedBatches() == Batches{1, 1, 1});
}

TEST_CASE("every landing reaches each handler exactly once and quiet Ticks call no handler", "[runtime][events]") {
    EventRuntimeProbe runtime;
    bootstrapOffStepBoundary(runtime);

    for (int i = 0; i < 110; i++) {
        REQUIRE(runtime.advanceSteps(1.0) == 1);
    }

    CHECK(runtime.entityCount() == 0);
    CHECK(runtime.system<LandingObserver>().getLandedBatches() == Batches(100, 1));
    CHECK(runtime.system<SpawnSystem>().getLandedBatches() == Batches(100, 1));

    std::vector<std::uint32_t> ordinals(100);
    std::iota(ordinals.begin(), ordinals.end(), 0U);
    CHECK(runtime.system<LandingSystem>().getSpawnOrdinals() == ordinals);
    CHECK(runtime.system<LandingObserver>().getDespawned().size() == 100);
    CHECK(runtime.system<PulseSystem>().getDespawned() == runtime.system<LandingObserver>().getDespawned());
    CHECK(runtime.system<SpawnSystem>().getPulseBatches() == Batches(109, PulseSystem::PULSES_PER_TICK));
    CHECK(runtime.system<SpawnSystem>().isPulseSequenceIntact());
}

TEST_CASE("a spawn published from the terminal slot reaches a regular-slot handler in the next Tick", "[runtime][events]") {
    EventRuntimeProbe runtime;
    bootstrapOffStepBoundary(runtime);

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingSystem>().getSpawnOrdinals().empty());

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingSystem>().getSpawnOrdinals() == std::vector<std::uint32_t>{0});

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingSystem>().getSpawnOrdinals() == std::vector<std::uint32_t>{0, 1});
}

TEST_CASE("a despawn published by a handler reaches every handler of its type one Tick later", "[runtime][events]") {
    EventRuntimeProbe runtime;
    const TechEngine::Entity first = spawnFirstEntity(runtime);

    REQUIRE(runtime.advanceSteps(2.0) == 2);

    CHECK(runtime.system<LandingObserver>().getDespawned().empty());
    CHECK(runtime.system<PulseSystem>().getDespawned().empty());

    REQUIRE(runtime.advanceSteps(1.0) == 1);

    CHECK(runtime.system<LandingObserver>().getDespawned() == std::vector<TechEngine::Entity>{first});
    CHECK(runtime.system<PulseSystem>().getDespawned() == std::vector<TechEngine::Entity>{first});
}

TEST_CASE("a burst of pulses every Tick arrives whole and in order, including across a catch-up", "[runtime][events]") {
    EventRuntimeProbe runtime;
    bootstrapOffStepBoundary(runtime);

    for (int i = 0; i < 5; i++) {
        REQUIRE(runtime.advanceSteps(1.0) == 1);
    }
    REQUIRE(runtime.advanceSteps(5.0) == 5);

    CHECK(runtime.system<SpawnSystem>().getPulseBatches() == Batches(9, PulseSystem::PULSES_PER_TICK));
    CHECK(runtime.system<SpawnSystem>().isPulseSequenceIntact());
}
