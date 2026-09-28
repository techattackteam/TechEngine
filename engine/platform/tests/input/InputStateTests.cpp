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
