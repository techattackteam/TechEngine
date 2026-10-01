#include <TechEngine/platform/input/InputState.hpp>

#include <catch2/catch_test_macros.hpp>

using TechEngine::InputEvent;
using TechEngine::InputKind;
using TechEngine::InputState;
using TechEngine::Key;
using TechEngine::MouseButton;

template<typename Event>
concept CarriesRawCode = requires(Event event) { event.code; };

template<typename State>
concept AnswersIntegerHeldQuery = requires(const State& state, int raw) { state.isHeld(raw); };

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

TEST_CASE("the input event and held state take engine identifiers, not raw integers", "[platform][input]") {
    STATIC_REQUIRE_FALSE(CarriesRawCode<InputEvent>);
    STATIC_REQUIRE_FALSE(AnswersIntegerHeldQuery<InputState>);
}

TEST_CASE("a key and a mouse button with the same ordinal are held independently", "[platform][input]") {
    InputState state;
    state.apply(focusEvent(true));

    state.apply(keyEvent(Key::Space, true));
    CHECK(state.isHeld(Key::Space));
    CHECK_FALSE(state.isHeld(MouseButton::Left));

    state.apply(buttonEvent(MouseButton::Left, true));
    state.apply(keyEvent(Key::Space, false));
    CHECK_FALSE(state.isHeld(Key::Space));
    CHECK(state.isHeld(MouseButton::Left));
}

TEST_CASE("the last key and the last mouse button are held and released like any other", "[platform][input]") {
    InputState state;
    state.apply(focusEvent(true));

    state.apply(keyEvent(Key::Menu, true));
    state.apply(buttonEvent(MouseButton::Extra5, true));
    CHECK(state.isHeld(Key::Menu));
    CHECK(state.isHeld(MouseButton::Extra5));
    CHECK(state.keys.count() == 1);
    CHECK(state.buttons.count() == 1);

    state.apply(keyEvent(Key::Menu, false));
    state.apply(buttonEvent(MouseButton::Extra5, false));
    CHECK(state.keys.none());
    CHECK(state.buttons.none());
}

TEST_CASE("key and mouse button presses are not held while unfocused", "[platform][input]") {
    InputState state;
    state.apply(keyEvent(Key::W, true));
    state.apply(buttonEvent(MouseButton::Left, true));
    CHECK_FALSE(state.isHeld(Key::W));
    CHECK_FALSE(state.isHeld(MouseButton::Left));

    state.apply(focusEvent(true));
    state.apply(focusEvent(false));
    state.apply(keyEvent(Key::W, true));
    state.apply(buttonEvent(MouseButton::Left, true));
    CHECK_FALSE(state.isHeld(Key::W));
    CHECK_FALSE(state.isHeld(MouseButton::Left));
}

TEST_CASE("apply reports whether the event changed held state", "[platform][input]") {
    InputState state;
    CHECK_FALSE(state.apply(focusEvent(false)));
    CHECK_FALSE(state.apply(keyEvent(Key::W, true)));
    CHECK_FALSE(state.apply(motionEvent(1.0, 1.0)));

    CHECK(state.apply(focusEvent(true)));
    CHECK(state.apply(keyEvent(Key::W, true)));
    CHECK_FALSE(state.apply(keyEvent(Key::W, true)));
    CHECK(state.apply(buttonEvent(MouseButton::Left, true)));
    CHECK(state.apply(motionEvent(2.0, -1.0)));
    CHECK(state.apply(keyEvent(Key::W, false)));
    CHECK_FALSE(state.apply(keyEvent(Key::W, false)));
    CHECK_FALSE(state.apply(keyEvent(Key::Unknown, true)));
    CHECK_FALSE(state.apply(buttonEvent(MouseButton::Unknown, true)));

    CHECK(state.apply(focusEvent(false)));
    CHECK_FALSE(state.apply(focusEvent(false)));
    CHECK_FALSE(state.apply(buttonEvent(MouseButton::Left, false)));
}

TEST_CASE("a duplicate focus value keeps held controls, look and the focus generation", "[platform][input]") {
    InputState state;
    state.apply(focusEvent(true));
    state.apply(keyEvent(Key::W, true));
    state.apply(buttonEvent(MouseButton::Right, true));
    state.apply(motionEvent(4.0, 3.0));
    REQUIRE(state.focusGeneration == 1);

    CHECK_FALSE(state.apply(focusEvent(true)));

    CHECK(state.focused);
    CHECK(state.isHeld(Key::W));
    CHECK(state.isHeld(MouseButton::Right));
    CHECK(state.lookX == 4.0);
    CHECK(state.lookY == 3.0);
    CHECK(state.focusGeneration == 1);
}

TEST_CASE("a focus transition clears held controls and the pointer baseline, and regain starts neutral", "[platform][input]") {
    InputState state;
    state.apply(focusEvent(true));
    state.apply(keyEvent(Key::W, true));
    state.apply(buttonEvent(MouseButton::Left, true));
    state.apply(motionEvent(5.0, -2.0));

    state.apply(focusEvent(false));
    CHECK(state.keys.none());
    CHECK(state.buttons.none());
    CHECK(state.lookX == 0.0);
    CHECK(state.lookY == 0.0);

    state.apply(focusEvent(true));
    CHECK(state.focused);
    CHECK(state.keys.none());
    CHECK(state.buttons.none());
    CHECK(state.lookX == 0.0);
    CHECK(state.lookY == 0.0);
    CHECK(state.focusGeneration == 3);
}

TEST_CASE("GLFW's synthetic releases after focus loss change nothing and leave no key stuck", "[platform][input]") {
    InputState state;
    state.apply(focusEvent(true));
    state.apply(keyEvent(Key::W, true));
    state.apply(buttonEvent(MouseButton::Left, true));

    REQUIRE(state.apply(focusEvent(false)));
    CHECK_FALSE(state.apply(keyEvent(Key::W, false)));
    CHECK_FALSE(state.apply(buttonEvent(MouseButton::Left, false)));

    state.apply(focusEvent(true));
    CHECK_FALSE(state.isHeld(Key::W));
    CHECK_FALSE(state.isHeld(MouseButton::Left));
}
