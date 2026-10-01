#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

using TechEngine::InputBuffer;
using TechEngine::InputEvent;
using TechEngine::InputFrame;
using TechEngine::InputKind;
using TechEngine::Key;
using TechEngine::MouseButton;

static InputEvent keyEvent(Key key, bool pressed) {
    return InputEvent{.kind = InputKind::Key, .key = key, .pressed = pressed};
}

static InputEvent buttonEvent(MouseButton button, bool pressed) {
    return InputEvent{.kind = InputKind::Button, .button = button, .pressed = pressed};
}

static InputEvent focusEvent(bool focused) {
    return InputEvent{.kind = InputKind::Focus, .pressed = focused};
}

static InputEvent motionEvent(double x, double y) {
    return InputEvent{.kind = InputKind::Motion, .x = x, .y = y};
}

TEST_CASE("both edges between two consumes arrive in capture order", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::A, true));
    input.publish(keyEvent(Key::A, false));

    input.consume(frame);

    REQUIRE(frame.events.size() == 3);
    CHECK_FALSE(frame.recovered);
    CHECK(frame.events[1].kind == InputKind::Key);
    CHECK(frame.events[1].pressed);
    CHECK_FALSE(frame.events[2].pressed);
    CHECK(frame.events[0].sequence < frame.events[1].sequence);
    CHECK(frame.events[1].sequence < frame.events[2].sequence);
    CHECK(frame.events[1].capturedAt <= frame.events[2].capturedAt);
    CHECK_FALSE(frame.held.isHeld(Key::A));
}

TEST_CASE("an event published after a consume belongs to the next consume", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::A, true));
    input.consume(frame);
    REQUIRE(frame.held.isHeld(Key::A));

    input.publish(keyEvent(Key::A, false));
    CHECK(frame.held.isHeld(Key::A));

    input.consume(frame);
    REQUIRE(frame.events.size() == 1);
    CHECK_FALSE(frame.held.isHeld(Key::A));
}

TEST_CASE("held state survives consumes with no new events", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::W, true));
    input.consume(frame);

    for (int i = 0; i < 5; i++) {
        input.consume(frame);
        CHECK(frame.events.empty());
        CHECK(frame.held.isHeld(Key::W));
    }
}

TEST_CASE("a dropped release cannot stick after overflow", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock, 2};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.consume(frame);

    input.publish(keyEvent(Key::A, true));
    input.publish(keyEvent(Key::B, true));
    input.publish(keyEvent(Key::A, false));
    input.publish(keyEvent(Key::C, true));
    input.consume(frame);

    CHECK(frame.recovered);
    CHECK(frame.events.empty());
    CHECK(frame.recoveryGeneration == 1);
    CHECK(frame.firstLostSequence == 2);
    CHECK(frame.lastLostSequence == 5);
    CHECK_FALSE(frame.held.isHeld(Key::A));
    CHECK(frame.held.isHeld(Key::B));
    CHECK(frame.held.isHeld(Key::C));
}

TEST_CASE("a full queue cannot hide recovery, and normal batches resume after it", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock, 1};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::A, true));

    input.consume(frame);
    CHECK(frame.recovered);
    CHECK(frame.held.focused);
    CHECK(frame.held.isHeld(Key::A));

    input.publish(keyEvent(Key::A, false));
    input.consume(frame);
    CHECK_FALSE(frame.recovered);
    REQUIRE(frame.events.size() == 1);
    CHECK_FALSE(frame.held.isHeld(Key::A));
}

TEST_CASE("focus changes start neutral and keys are ignored while unfocused", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::A, true));
    input.consume(frame);
    REQUIRE(frame.held.isHeld(Key::A));

    input.publish(focusEvent(false));
    input.publish(keyEvent(Key::B, true));
    input.publish(focusEvent(true));
    input.consume(frame);

    CHECK(frame.held.focused);
    CHECK(frame.held.focusGeneration == 3);
    CHECK(frame.held.keys.none());
}

TEST_CASE("a consume keeps only the events that changed held state, with their capture sequence", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(keyEvent(Key::W, true));
    input.publish(focusEvent(true));
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::W, true));
    input.publish(keyEvent(Key::W, false));
    input.publish(keyEvent(Key::W, false));

    input.consume(frame);

    REQUIRE(frame.events.size() == 3);
    CHECK(frame.events[0].kind == InputKind::Focus);
    CHECK(frame.events[0].sequence == 2);
    CHECK(frame.events[1].kind == InputKind::Key);
    CHECK(frame.events[1].pressed);
    CHECK(frame.events[1].sequence == 4);
    CHECK(frame.events[2].kind == InputKind::Key);
    CHECK_FALSE(frame.events[2].pressed);
    CHECK(frame.events[2].sequence == 5);
    CHECK(frame.held.focusGeneration == 1);
}

TEST_CASE("GLFW's post-loss synthetic releases are not kept and no control stays held", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::W, true));
    input.publish(buttonEvent(MouseButton::Left, true));
    input.consume(frame);
    REQUIRE(frame.held.isHeld(Key::W));

    input.publish(focusEvent(false));
    input.publish(keyEvent(Key::W, false));
    input.publish(buttonEvent(MouseButton::Left, false));
    input.consume(frame);

    REQUIRE(frame.events.size() == 1);
    CHECK(frame.events[0].kind == InputKind::Focus);
    CHECK_FALSE(frame.events[0].pressed);
    CHECK(frame.held.keys.none());
    CHECK(frame.held.buttons.none());

    input.publish(focusEvent(true));
    input.consume(frame);

    REQUIRE(frame.events.size() == 1);
    CHECK(frame.events[0].pressed);
    CHECK_FALSE(frame.held.isHeld(Key::W));
    CHECK_FALSE(frame.held.isHeld(MouseButton::Left));
}

TEST_CASE("a duplicate focus between consumes keeps a held key held", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(keyEvent(Key::W, true));
    input.consume(frame);

    input.publish(focusEvent(true));
    input.consume(frame);

    CHECK(frame.events.empty());
    CHECK(frame.held.isHeld(Key::W));
    CHECK(input.presentationState().isHeld(Key::W));
}

TEST_CASE("presentation look accumulates independently of simulation consumes", "[platform][input]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock};
    InputFrame frame;
    input.publish(focusEvent(true));
    input.publish(motionEvent(1.0, 2.0));
    input.consume(frame);
    input.publish(motionEvent(3.0, 4.0));

    const TechEngine::InputState look = input.presentationState();
    CHECK(look.lookX == 4.0);
    CHECK(look.lookY == 6.0);
    CHECK(look.sequence == 3);
    CHECK(frame.held.lookX == 1.0);

    input.consume(frame);
    CHECK(frame.held.lookX == 4.0);
    CHECK(input.presentationState().lookX == 4.0);

    input.publish(focusEvent(false));
    const TechEngine::InputState reset = input.presentationState();
    CHECK(reset.lookX == 0.0);
    CHECK(reset.focusGeneration == 2);
}

TEST_CASE("concurrent input delivery recovers coherently while presentation reads independently", "[platform][input][concurrency]") {
    const TechEngine::Clock clock;
    InputBuffer input{clock, 4};
    std::atomic<bool> done = false;
    std::atomic<bool> coherent = true;
    std::jthread producer{[&] {
        input.publish(focusEvent(true));
        for (int i = 0; i < 20000; i++) {
            input.publish(keyEvent(Key::A, true));
            input.publish(motionEvent(1.0, 2.0));
            input.publish(keyEvent(Key::A, false));
        }
        done = true;
    }};
    std::jthread presentation{[&] {
        std::uint64_t previous = 0;
        while (!done.load()) {
            const auto state = input.presentationState();
            if (state.sequence < previous || state.lookY != state.lookX * 2.0) {
                coherent = false;
            }
            previous = state.sequence;
        }
    }};
    InputFrame frame;
    std::uint64_t previous = 0;
    do {
        input.consume(frame);
        if (frame.held.sequence < previous || frame.held.lookY != frame.held.lookX * 2.0) {
            coherent = false;
        }
        previous = frame.held.sequence;
    } while (!done.load());
    producer.join();
    presentation.join();
    input.consume(frame);
    CHECK(coherent.load());
    CHECK_FALSE(frame.held.isHeld(Key::A));
    CHECK(frame.held.lookX == 20000.0);
}
