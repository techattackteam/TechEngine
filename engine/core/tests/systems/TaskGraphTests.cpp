#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

template<std::size_t Index>
struct GraphComponent {
    std::uint32_t value = 0;
};

static constexpr std::array<std::string_view, 5> GRAPH_SYSTEM_NAMES{
    "FirstSystem",
    "SecondSystem",
    "ThirdSystem",
    "FourthSystem",
    "TerminalSystem",
};

static int g_graphConstructed = 0;
static int g_graphInitialized = 0;
static int g_graphDestroyed = 0;

template<std::size_t Index>
class GraphSystem final : public TechEngine::ISystem {
public:
    GraphSystem() {
        g_graphConstructed++;
    }

    ~GraphSystem() override {
        g_graphDestroyed++;
    }

    void init(TechEngine::ScheduleRegistration&) override {
        g_graphInitialized++;
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        static_assert(Index < GRAPH_SYSTEM_NAMES.size());
        return GRAPH_SYSTEM_NAMES[Index];
    }
};

using FirstSystem = GraphSystem<0>;
using SecondSystem = GraphSystem<1>;
using ThirdSystem = GraphSystem<2>;
using FourthSystem = GraphSystem<3>;
using TerminalSystem = GraphSystem<4>;

static std::vector<std::string> g_graphLogMessages;

static void captureGraphLog(const TechEngine::LogRecord& record) {
    g_graphLogMessages.emplace_back(record.message);
}

class GraphLogCaptureGuard {
private:
    TechEngine::Level m_previousLevel;

public:
    GraphLogCaptureGuard() : m_previousLevel(TechEngine::minLevel()) {
        g_graphLogMessages.clear();
        TechEngine::setMinLevel(TechEngine::Level::Trace);
        (void)TechEngine::addLogSink(&captureGraphLog);
    }

    ~GraphLogCaptureGuard() {
        (void)TechEngine::removeLogSink(&captureGraphLog);
        TechEngine::setMinLevel(m_previousLevel);
        g_graphLogMessages.clear();
    }

    GraphLogCaptureGuard(const GraphLogCaptureGuard&) = delete;

    GraphLogCaptureGuard& operator=(const GraphLogCaptureGuard&) = delete;
};

template<std::size_t... Indices>
static void registerGraphComponents(TechEngine::ComponentRegistry& registry, std::index_sequence<Indices...>) {
    (registry.registerComponent<GraphComponent<Indices>>("Test.GraphComponent." + std::to_string(Indices)), ...);
}

template<typename T>
static std::optional<std::size_t> findSystemLevel(const TechEngine::TaskGraph& graph) {
    for (std::size_t levelIndex = 0; levelIndex < graph.getLevels().size(); levelIndex++) {
        for (const TechEngine::TaskGraphNode& node: graph.getLevels()[levelIndex]) {
            if (node.systemType == std::type_index(typeid(T))) {
                return levelIndex;
            }
        }
    }
    return std::nullopt;
}

template<typename T>
static std::size_t countSystemNodes(const TechEngine::TaskGraph& graph) {
    std::size_t count = 0;
    for (const TechEngine::TaskGraphLevel& level: graph.getLevels()) {
        for (const TechEngine::TaskGraphNode& node: level) {
            if (node.systemType == std::type_index(typeid(T))) {
                count++;
            }
        }
    }
    return count;
}

static std::size_t countExactLog(const std::string_view expected) {
    std::size_t count = 0;
    for (const std::string& message: g_graphLogMessages) {
        if (message == expected) {
            count++;
        }
    }
    return count;
}

TEST_CASE("an empty schedule builds an empty graph and freezes registration", "[core][systems][task-graph]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().empty());
    REQUIRE(schedule.frozen());
    REQUIRE_THROWS_AS(schedule.add<FirstSystem>(), TechEngineTests::AssertFired);
}

TEST_CASE("independent systems occupy one level exactly once", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>();
    schedule.add<SecondSystem>();
    schedule.add<ThirdSystem>();

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().size() == 1);
    REQUIRE(graph.getLevels().front().size() == 3);
    REQUIRE(countSystemNodes<FirstSystem>(graph) == 1);
    REQUIRE(countSystemNodes<SecondSystem>(graph) == 1);
    REQUIRE(countSystemNodes<ThirdSystem>(graph) == 1);
    for (const TechEngine::TaskGraphNode& node: graph.getLevels().front()) {
        REQUIRE(node.system != nullptr);
    }
}

TEST_CASE("a schedule frozen by the composition root can still be built", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>();
    schedule.freeze();

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().size() == 1);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
}

TEST_CASE("readers of the same component remain on the same level", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{});
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{});

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 0);
}

TEST_CASE("conflicts cross access-mask word boundaries without touching neighboring components", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<65>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<64>>, TechEngine::Read<>>{}).setPriority(10);
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<64>>>{}).setPriority(20);
    schedule.add<ThirdSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<63>>>{});

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<ThirdSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 1);
}

TEST_CASE("conflicts form the worked priority-ordered graph levels", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<4>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<GraphComponent<1>>>{}).setPriority(10);
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<1>>, TechEngine::Read<>>{}).setPriority(20);
    schedule.add<ThirdSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<GraphComponent<2>>>{}).setPriority(30);
    schedule.add<FourthSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<3>>, TechEngine::Read<GraphComponent<0>>>{}).setPriority(40);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().size() == 3);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 1);
    REQUIRE(findSystemLevel<ThirdSystem>(graph) == 1);
    REQUIRE(findSystemLevel<FourthSystem>(graph) == 2);
}

TEST_CASE("lower numeric priority wins regardless of registration order", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{}).setPriority(20);
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{}).setPriority(-10);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<SecondSystem>(graph) == 0);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 1);
}

TEST_CASE("explicit order overrides priority for a conflicting pair", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{}).setPriority(20).before<SecondSystem>();
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{}).setPriority(10);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 1);
}

TEST_CASE("explicit order creates an edge between otherwise independent systems", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>().after<SecondSystem>();
    schedule.add<SecondSystem>();

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<SecondSystem>(graph) == 0);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 1);
}

TEST_CASE("equivalent order declarations produce one dependency", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>().before<SecondSystem>();
    schedule.add<SecondSystem>().after<FirstSystem>();

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().size() == 2);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 1);
    REQUIRE(countSystemNodes<FirstSystem>(graph) == 1);
    REQUIRE(countSystemNodes<SecondSystem>(graph) == 1);
}

TEST_CASE("equal-priority conflicts fail without freezing the schedule", "[core][systems][task-graph]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{});
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{});

    REQUIRE_THROWS_AS(TechEngine::TaskGraph(schedule), TechEngineTests::AssertFired);
    REQUIRE_FALSE(schedule.frozen());

    schedule.add<ThirdSystem>();
    REQUIRE(schedule.getEntries().size() == 3);
}

TEST_CASE("an invalid graph does not construct extra systems and cleans up selected instances", "[core][systems][task-graph]") {
    const TechEngineTests::FatalAssertGuard guard;
    g_graphConstructed = 0;
    g_graphInitialized = 0;
    g_graphDestroyed = 0;
    {
        TechEngine::ComponentRegistry registry;
        registerGraphComponents(registry, std::make_index_sequence<1>{});
        TechEngine::Schedule schedule(registry);
        schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{});
        schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{});

        REQUIRE(g_graphConstructed == 2);
        REQUIRE(g_graphInitialized == 2);
        REQUIRE_THROWS_AS(TechEngine::TaskGraph(schedule), TechEngineTests::AssertFired);
        REQUIRE(g_graphConstructed == 2);
        REQUIRE(g_graphInitialized == 2);
        REQUIRE(g_graphDestroyed == 0);
        REQUIRE_FALSE(schedule.frozen());
    }
    REQUIRE(g_graphDestroyed == 2);
}

TEST_CASE("explicit order resolves an equal-priority conflict", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{}).after<SecondSystem>();
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{});

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(findSystemLevel<SecondSystem>(graph) == 0);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 1);
}

TEST_CASE("a cycle names every involved system and leaves the schedule mutable", "[core][systems][task-graph]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>().before<SecondSystem>();
    schedule.add<SecondSystem>().before<ThirdSystem>();
    schedule.add<ThirdSystem>().before<FirstSystem>();
    std::optional<TechEngineTests::AssertFired> failure;

    try {
        (void)TechEngine::TaskGraph(schedule);
    } catch (const TechEngineTests::AssertFired& fired) {
        failure = fired;
    }

    REQUIRE(failure.has_value());
    REQUIRE(failure->message.find("FirstSystem") != std::string::npos);
    REQUIRE(failure->message.find("SecondSystem") != std::string::npos);
    REQUIRE(failure->message.find("ThirdSystem") != std::string::npos);
    REQUIRE_FALSE(schedule.frozen());
    schedule.add<FourthSystem>();
}

TEST_CASE("an order constraint naming an unregistered system is rejected", "[core][systems][task-graph]") {
    const TechEngineTests::FatalAssertGuard guard;
    TechEngine::ComponentRegistry registry;
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>().before<SecondSystem>();

    REQUIRE_THROWS_AS(TechEngine::TaskGraph(schedule), TechEngineTests::AssertFired);
    REQUIRE_FALSE(schedule.frozen());

    schedule.add<SecondSystem>();
    const TechEngine::TaskGraph graph(schedule);
    REQUIRE(schedule.frozen());
    REQUIRE(graph.getLevels().size() == 2);
}

TEST_CASE("the terminal system is a final singleton level after every regular level", "[core][systems][task-graph]") {
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<1>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<TerminalSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{}).setSlot(TechEngine::Slot::Terminal);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{}).setPriority(10);
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<0>>>{}).setPriority(20);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(graph.getLevels().size() == 3);
    REQUIRE(findSystemLevel<FirstSystem>(graph) == 0);
    REQUIRE(findSystemLevel<SecondSystem>(graph) == 1);
    REQUIRE(findSystemLevel<TerminalSystem>(graph) == 2);
    REQUIRE(graph.getLevels().back().size() == 1);
}

TEST_CASE("every conflict-derived edge is logged with its chosen direction", "[core][systems][task-graph]") {
    const GraphLogCaptureGuard guard;
    TechEngine::ComponentRegistry registry;
    registerGraphComponents(registry, std::make_index_sequence<2>{});
    TechEngine::Schedule schedule(registry);
    schedule.add<FirstSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<0>>, TechEngine::Read<>>{}).setPriority(10);
    schedule.add<SecondSystem>(TechEngine::DeclareAccess<TechEngine::Write<GraphComponent<1>>, TechEngine::Read<GraphComponent<0>>>{}).setPriority(20);
    schedule.add<ThirdSystem>(TechEngine::DeclareAccess<TechEngine::Write<>, TechEngine::Read<GraphComponent<1>>>{}).setPriority(30);

    const TechEngine::TaskGraph graph(schedule);

    REQUIRE(g_graphLogMessages.size() == 2);
    REQUIRE(countExactLog("Task graph conflict: FirstSystem -> SecondSystem") == 1);
    REQUIRE(countExactLog("Task graph conflict: SecondSystem -> ThirdSystem") == 1);
}
