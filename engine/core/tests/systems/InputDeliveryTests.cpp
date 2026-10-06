#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/core/systems/InputNotification.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct InputDeliveryValue {
    int value = 0;
};

struct InputDeliveryHit {
    static constexpr std::string_view tag = "Test.InputDeliveryHit";

    std::uint32_t amount;
};

enum class InputDeliveryAccessMode { None, Write, UndeclaredRead };

struct InputDeliveryTestState {
    std::vector<std::string> trace;
    std::vector<TechEngine::InputNotification> received;
    TechEngine::Entity target;
    InputDeliveryAccessMode accessMode = InputDeliveryAccessMode::None;
    bool publishHit = false;
    bool throwFromHandler = false;
};

static InputDeliveryTestState* g_inputDeliveryState = nullptr;

class InputDeliveryStateGuard {
public:
    explicit InputDeliveryStateGuard(InputDeliveryTestState& state) {
        REQUIRE(g_inputDeliveryState == nullptr);
        g_inputDeliveryState = &state;
    }

    ~InputDeliveryStateGuard() {
        g_inputDeliveryState = nullptr;
    }

    InputDeliveryStateGuard(const InputDeliveryStateGuard&) = delete;

    InputDeliveryStateGuard& operator=(const InputDeliveryStateGuard&) = delete;
};

static std::string handled(const int reader, const int handler, const std::uint64_t sequence) {
    return "reader" + std::to_string(reader) + " handler" + std::to_string(handler) + " #" + std::to_string(sequence);
}

static std::string ticked(const int reader) {
    return "reader" + std::to_string(reader) + " tick";
}

template<int Reader>
class InputReaderSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene&, const TechEngine::InputNotification& input) {
            g_inputDeliveryState->trace.push_back(handled(Reader, 1, input.sequence));
            if constexpr (Reader == 1) {
                g_inputDeliveryState->received.push_back(input);
            }
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_inputDeliveryState->trace.push_back(ticked(Reader));
    }

    std::string_view name() const override {
        return "InputReaderSystem";
    }
};

using FirstInputReader = InputReaderSystem<1>;
using SecondInputReader = InputReaderSystem<2>;

class TwoHandlerInputSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene&, const TechEngine::InputNotification& input) {
            g_inputDeliveryState->trace.push_back(handled(3, 1, input.sequence));
        });
        registration.onInput([](TechEngine::Scene&, const TechEngine::InputNotification& input) {
            g_inputDeliveryState->trace.push_back(handled(3, 2, input.sequence));
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_inputDeliveryState->trace.push_back(ticked(3));
    }

    std::string_view name() const override {
        return "TwoHandlerInputSystem";
    }
};

class InputBlindSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_inputDeliveryState->trace.push_back(ticked(4));
    }

    std::string_view name() const override {
        return "InputBlindSystem";
    }
};

class HitPublisherSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext&) override {
        if (g_inputDeliveryState->publishHit) {
            scene.publish<InputDeliveryHit>(1U);
        }
    }

    std::string_view name() const override {
        return "HitPublisherSystem";
    }
};

class EventAndInputSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene&, const TechEngine::InputNotification& input) {
            g_inputDeliveryState->trace.push_back("input #" + std::to_string(input.sequence));
        });
        registration.onEvent<InputDeliveryHit>([](TechEngine::Scene&, std::span<const InputDeliveryHit>) {
            g_inputDeliveryState->trace.push_back("event");
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_inputDeliveryState->trace.push_back("tick");
    }

    std::string_view name() const override {
        return "EventAndInputSystem";
    }
};

class InputAccessSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene& scene, const TechEngine::InputNotification&) {
            g_inputDeliveryState->trace.push_back("input");
            switch (g_inputDeliveryState->accessMode) {
                case InputDeliveryAccessMode::None:
                    break;
                case InputDeliveryAccessMode::Write:
                    (void)scene.getComponent<TechEngine::Transform>(g_inputDeliveryState->target);
                    break;
                case InputDeliveryAccessMode::UndeclaredRead:
                    (void)scene.hasComponent<InputDeliveryValue>(g_inputDeliveryState->target);
                    break;
            }
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
    }

    std::string_view name() const override {
        return "InputAccessSystem";
    }
};

class ThrowingInputSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration& registration) override {
        registration.onInput([](TechEngine::Scene& scene, const TechEngine::InputNotification&) {
            if (!g_inputDeliveryState->throwFromHandler) {
                return;
            }
            scene.getCommands().despawn(g_inputDeliveryState->target);
            throw std::runtime_error("input handler failure");
        });
    }

    void tick(TechEngine::Scene&, const TechEngine::SimulationContext&) override {
        g_inputDeliveryState->trace.push_back("tick");
    }

    std::string_view name() const override {
        return "ThrowingInputSystem";
    }
};

class InputDeliveryBarrier final : public TechEngine::TickBarrierServices {
public:
    int calls = 0;

    void assignNetIds(TechEngine::Scene&, std::span<const TechEngine::Entity>) override {
        calls++;
    }
};

class InputDeliveryFixture {
public:
    TechEngine::ComponentRegistry registry;
    TechEngine::EventRegistry events;
    TechEngine::Scene scene;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files;
    TechEngine::JobSystem jobs;
    TechEngine::Clock clock;
    TechEngine::EngineContext engine;
    TechEngine::InputFrame input;
    TechEngine::SimulationContext context;
    InputDeliveryBarrier barrier;
    std::uint64_t sequence = 0;

    InputDeliveryFixture() : scene(registry), files(mounts), jobs(1), engine{files, jobs, clock}, context{.fixedDeltaTime = 1.0 / 60.0, .tick = 1, .input = input, .engine = engine} {
        TechEngineTests::registerBuiltInSceneComponents(registry);
        registry.registerComponent<InputDeliveryValue>("Test.InputDeliveryValue");
    }

    void capture(TechEngine::InputEvent event) {
        event.sequence = ++sequence;
        input.held.apply(event);
        input.events.push_back(event);
    }

    // Mirrors an overflowing InputBuffer: a lost event still changes the held state the frame recovers to.
    void lose(TechEngine::InputEvent event) {
        event.sequence = ++sequence;
        input.held.apply(event);
        if (!input.recovered) {
            input.recovered = true;
            input.firstLostSequence = event.sequence;
        }
        input.lastLostSequence = event.sequence;
    }

    void runTick(TechEngine::SerialExecutor& executor) {
        executor.execute(scene, context, barrier);
        input.events.clear();
        input.recovered = false;
        context.tick++;
    }
};

static TechEngine::InputEvent keyEvent(const TechEngine::Key key, const bool pressed) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Key, .key = key, .pressed = pressed};
}

static TechEngine::InputEvent buttonEvent(const TechEngine::MouseButton button, const bool pressed) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Button, .button = button, .pressed = pressed};
}

static TechEngine::InputEvent motionEvent(const double x, const double y) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Motion, .x = x, .y = y};
}

static TechEngine::InputEvent focusEvent(const bool focused) {
    return TechEngine::InputEvent{.kind = TechEngine::InputKind::Focus, .pressed = focused};
}

TEST_CASE("a press and release captured between two Ticks both reach the handler in capture order", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.runTick(executor);
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.capture(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{ticked(1), handled(1, 1, 1), handled(1, 1, 2), ticked(1), ticked(1)});
    REQUIRE(state.received.size() == 2);
    CHECK(state.received[0].kind == TechEngine::InputNotificationKind::Key);
    CHECK(state.received[0].key == TechEngine::Key::W);
    CHECK(state.received[0].pressed);
    CHECK(state.received[1].kind == TechEngine::InputNotificationKind::Key);
    CHECK(state.received[1].key == TechEngine::Key::W);
    CHECK_FALSE(state.received[1].pressed);
}

TEST_CASE("key, button, motion and focus events arrive interleaved in sequence order", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::Space, true));
    fixture.capture(motionEvent(12.5, -3.0));
    fixture.capture(buttonEvent(TechEngine::MouseButton::Right, true));
    fixture.capture(focusEvent(false));
    fixture.runTick(executor);

    REQUIRE(state.received.size() == 5);
    for (std::size_t i = 0; i < state.received.size(); i++) {
        CHECK(state.received[i].sequence == i + 1);
    }
    CHECK(state.received[0].kind == TechEngine::InputNotificationKind::Focus);
    CHECK(state.received[0].pressed);
    CHECK(state.received[1].kind == TechEngine::InputNotificationKind::Key);
    CHECK(state.received[1].key == TechEngine::Key::Space);
    CHECK(state.received[1].pressed);
    CHECK(state.received[2].kind == TechEngine::InputNotificationKind::Motion);
    CHECK(state.received[2].x == 12.5);
    CHECK(state.received[2].y == -3.0);
    CHECK(state.received[3].kind == TechEngine::InputNotificationKind::Button);
    CHECK(state.received[3].button == TechEngine::MouseButton::Right);
    CHECK(state.received[3].pressed);
    CHECK(state.received[4].kind == TechEngine::InputNotificationKind::Focus);
    CHECK_FALSE(state.received[4].pressed);
}

TEST_CASE("two readers each receive the whole batch at their own slot, including the terminal slot", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>().before<InputBlindSystem>();
    schedule.add<InputBlindSystem>();
    schedule.add<SecondInputReader>().setSlot(TechEngine::Slot::Terminal);
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(keyEvent(TechEngine::Key::A, true));
    fixture.capture(keyEvent(TechEngine::Key::A, false));
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{handled(1, 1, 1), handled(1, 1, 2), ticked(1), ticked(4), handled(2, 1, 1), handled(2, 1, 2), ticked(2)});
}

TEST_CASE("several handlers on one system run in declaration order for each event", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<TwoHandlerInputSystem>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(keyEvent(TechEngine::Key::D, true));
    fixture.capture(keyEvent(TechEngine::Key::D, false));
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{handled(3, 1, 1), handled(3, 2, 1), handled(3, 1, 2), handled(3, 2, 2), ticked(3)});
}

TEST_CASE("input handlers run after the system's Scene event handlers and before its tick", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    fixture.events.registerEvent<InputDeliveryHit>();
    fixture.scene.buildEventStreams(fixture.events);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<HitPublisherSystem>().before<EventAndInputSystem>();
    schedule.add<EventAndInputSystem>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    state.publishHit = true;
    fixture.capture(keyEvent(TechEngine::Key::E, true));
    fixture.runTick(executor);
    state.publishHit = false;
    fixture.capture(keyEvent(TechEngine::Key::E, false));
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{"input #1", "tick", "event", "input #2", "tick"});
}

TEST_CASE("a declared write from an input handler passes the access check", "[core][systems][input]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    state.accessMode = InputDeliveryAccessMode::Write;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<InputAccessSystem>(TechEngine::DeclareAccess<TechEngine::Write<TechEngine::Transform>, TechEngine::Read<>>{});
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(keyEvent(TechEngine::Key::S, true));
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{"input"});
    REQUIRE(TechEngineTests::g_fired.empty());
}

TEST_CASE("undeclared access from an input handler fires the debug assertion without corrupting the execution scope", "[core][systems][input]") {
    const TechEngineTests::AssertHandlerGuard assertGuard;
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    state.accessMode = InputDeliveryAccessMode::UndeclaredRead;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<InputAccessSystem>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(keyEvent(TechEngine::Key::S, true));
    fixture.runTick(executor);

#if TE_ASSERT_DEV
    REQUIRE(TechEngineTests::g_fired == std::vector<TechEngine::AssertKind>{TechEngine::AssertKind::Assert});
#else
    REQUIRE(TechEngineTests::g_fired.empty());
#endif
    state.accessMode = InputDeliveryAccessMode::None;
    fixture.capture(keyEvent(TechEngine::Key::S, false));
    fixture.runTick(executor);
}

TEST_CASE("a throwing input handler skips its tick, discards pending commands and skips the barrier", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    state.target = fixture.scene.createEntity();
    state.throwFromHandler = true;
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<ThrowingInputSystem>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(keyEvent(TechEngine::Key::Q, true));
    REQUIRE_THROWS_AS(executor.execute(fixture.scene, fixture.context, fixture.barrier), std::runtime_error);

    REQUIRE(state.trace.empty());
    REQUIRE(fixture.scene.contains(state.target));
    REQUIRE(fixture.barrier.calls == 0);

    state.throwFromHandler = false;
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{"tick"});
    REQUIRE(fixture.scene.contains(state.target));
    REQUIRE(fixture.barrier.calls == 1);
}

TEST_CASE("an empty batch with nothing held calls no input handler but still runs the tick", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<TwoHandlerInputSystem>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.runTick(executor);
    fixture.runTick(executor);

    REQUIRE(state.trace == std::vector<std::string>{ticked(3), ticked(3)});
}

static std::vector<TechEngine::InputNotificationKind> kindsOf(const std::vector<TechEngine::InputNotification>& received) {
    std::vector<TechEngine::InputNotificationKind> kinds;
    for (const TechEngine::InputNotification& notification: received) {
        kinds.push_back(notification.kind);
    }
    return kinds;
}

TEST_CASE("a press and release in one Tick deliver both edges and no hold notification", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.capture(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);
    fixture.runTick(executor);

    using enum TechEngine::InputNotificationKind;
    REQUIRE(kindsOf(state.received) == std::vector{Focus, Key, Key});
    CHECK(state.received[1].pressed);
    CHECK_FALSE(state.received[2].pressed);
}

TEST_CASE("a held key reports one hold notification per Tick, including quiet Ticks", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Focus, Key, KeyHold});
    CHECK(state.received[2].key == TechEngine::Key::W);

    for (int i = 0; i < 3; i++) {
        state.received.clear();
        fixture.runTick(executor);
        REQUIRE(kindsOf(state.received) == std::vector{KeyHold});
        CHECK(state.received[0].key == TechEngine::Key::W);
    }

    state.received.clear();
    fixture.capture(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Key});
    CHECK_FALSE(state.received[0].pressed);

    state.received.clear();
    state.trace.clear();
    fixture.runTick(executor);
    CHECK(state.received.empty());
    CHECK(state.trace == std::vector<std::string>{ticked(1)});
}

TEST_CASE("hold notifications follow captured events in ascending key order, then button order", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(focusEvent(true));
    fixture.capture(buttonEvent(TechEngine::MouseButton::Right, true));
    fixture.capture(keyEvent(TechEngine::Key::D, true));
    fixture.capture(buttonEvent(TechEngine::MouseButton::Left, true));
    fixture.capture(keyEvent(TechEngine::Key::A, true));
    fixture.capture(keyEvent(TechEngine::Key::Space, true));
    fixture.capture(motionEvent(1.0, 1.0));
    fixture.runTick(executor);

    using enum TechEngine::InputNotificationKind;
    REQUIRE(kindsOf(state.received) == std::vector{Focus, Button, Key, Button, Key, Key, Motion, KeyHold, KeyHold, KeyHold, ButtonHold, ButtonHold});
    for (std::size_t i = 0; i < 7; i++) {
        CHECK(state.received[i].sequence == i + 1);
    }
    CHECK(state.received[7].key == TechEngine::Key::Space);
    CHECK(state.received[8].key == TechEngine::Key::A);
    CHECK(state.received[9].key == TechEngine::Key::D);
    CHECK(state.received[10].button == TechEngine::MouseButton::Left);
    CHECK(state.received[11].button == TechEngine::MouseButton::Right);
}

TEST_CASE("every reader receives the hold notifications at its own slot before its tick", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>().before<InputBlindSystem>();
    schedule.add<InputBlindSystem>();
    schedule.add<SecondInputReader>().setSlot(TechEngine::Slot::Terminal);
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);
    state.trace.clear();
    fixture.runTick(executor);

    REQUIRE(state.trace.size() == 5);
    CHECK(state.trace[0].starts_with("reader1 handler1"));
    CHECK(state.trace[1] == ticked(1));
    CHECK(state.trace[2] == ticked(4));
    CHECK(state.trace[3].starts_with("reader2 handler1"));
    CHECK(state.trace[4] == ticked(2));
}

TEST_CASE("a focus loss ends hold notifications in the Tick that captured it", "[core][systems][input]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.capture(buttonEvent(TechEngine::MouseButton::Left, true));
    fixture.runTick(executor);
    state.received.clear();

    fixture.capture(focusEvent(false));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Focus});
    CHECK_FALSE(state.received[0].pressed);

    state.received.clear();
    fixture.capture(focusEvent(true));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Focus});
    CHECK(state.received[0].pressed);
}

TEST_CASE("an overflow reaches the handler as one recovery notice with the lost range, before the recovered holds and without invented edges", "[core][systems][input][overflow]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);
    state.received.clear();

    fixture.lose(keyEvent(TechEngine::Key::W, false));
    fixture.lose(keyEvent(TechEngine::Key::A, true));
    fixture.lose(buttonEvent(TechEngine::MouseButton::Left, true));
    fixture.runTick(executor);

    REQUIRE(kindsOf(state.received) == std::vector{Recovered, KeyHold, ButtonHold});
    CHECK(state.received[0].firstLostSequence == 3);
    CHECK(state.received[0].lastLostSequence == 5);
    CHECK(state.received[0].pressed);
    CHECK(state.received[0].sequence == 0);
    CHECK(state.received[1].key == TechEngine::Key::A);
    CHECK(state.received[2].button == TechEngine::MouseButton::Left);
}

TEST_CASE("a recovery that leaves nothing held still reaches the handler before its tick", "[core][systems][input][overflow]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);
    state.received.clear();
    state.trace.clear();

    fixture.lose(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);

    REQUIRE(kindsOf(state.received) == std::vector{Recovered});
    CHECK(state.received[0].firstLostSequence == 3);
    CHECK(state.received[0].lastLostSequence == 3);
    CHECK(state.received[0].pressed);
    CHECK(state.trace == std::vector<std::string>{handled(1, 1, 0), ticked(1)});
    CHECK_FALSE(fixture.input.held.isHeld(TechEngine::Key::W));
}

TEST_CASE("a focus loss inside the lost range shows in the notice and ends the holds", "[core][systems][input][overflow]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.capture(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);
    state.received.clear();

    fixture.lose(focusEvent(false));
    fixture.lose(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);

    REQUIRE(kindsOf(state.received) == std::vector{Recovered});
    CHECK_FALSE(state.received[0].pressed);
    CHECK(state.received[0].firstLostSequence == 3);
    CHECK(state.received[0].lastLostSequence == 4);

    state.received.clear();
    fixture.runTick(executor);
    CHECK(state.received.empty());
}

TEST_CASE("the Tick after a recovery has no notice, its holds resume, and captured sequences continue after the lost range", "[core][systems][input][overflow]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>();
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.runTick(executor);
    state.received.clear();

    fixture.lose(keyEvent(TechEngine::Key::W, true));
    fixture.lose(keyEvent(TechEngine::Key::D, true));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Recovered, KeyHold, KeyHold});
    CHECK(state.received[1].key == TechEngine::Key::D);
    CHECK(state.received[2].key == TechEngine::Key::W);

    state.received.clear();
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{KeyHold, KeyHold});

    state.received.clear();
    fixture.capture(keyEvent(TechEngine::Key::W, false));
    fixture.runTick(executor);
    REQUIRE(kindsOf(state.received) == std::vector{Key, KeyHold});
    CHECK(state.received[0].sequence == 4);
    CHECK_FALSE(state.received[0].pressed);
    CHECK(state.received[1].key == TechEngine::Key::D);
}

TEST_CASE("every reader receives the recovery notice at its own slot, including the terminal slot", "[core][systems][input][overflow]") {
    InputDeliveryFixture fixture;
    InputDeliveryTestState state;
    const InputDeliveryStateGuard stateGuard(state);
    TechEngine::Schedule schedule(fixture.registry);
    schedule.add<FirstInputReader>().before<InputBlindSystem>();
    schedule.add<InputBlindSystem>();
    schedule.add<SecondInputReader>().setSlot(TechEngine::Slot::Terminal);
    const TechEngine::TaskGraph graph(schedule, fixture.events);
    TechEngine::SerialExecutor executor(graph);
    using enum TechEngine::InputNotificationKind;

    fixture.capture(focusEvent(true));
    fixture.runTick(executor);
    state.received.clear();
    state.trace.clear();

    fixture.lose(keyEvent(TechEngine::Key::W, true));
    fixture.runTick(executor);

    REQUIRE(kindsOf(state.received) == std::vector{Recovered, KeyHold});
    CHECK(state.trace == std::vector<std::string>{handled(1, 1, 0), handled(1, 1, 0), ticked(1), ticked(4), handled(2, 1, 0), handled(2, 1, 0), ticked(2)});
}
