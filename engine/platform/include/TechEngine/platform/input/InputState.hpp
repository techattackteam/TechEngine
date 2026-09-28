#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/platform/input/Key.hpp>
#include <TechEngine/platform/input/MouseButton.hpp>

#include <bitset>
#include <cstdint>

namespace TechEngine {
    enum class InputKind : std::uint8_t { Key, Button, Motion, Focus };

    struct InputEvent {
        InputKind kind = InputKind::Motion;
        Key key{};
        MouseButton button{};
        bool pressed = false;
        double x = 0.0;
        double y = 0.0;
        std::uint64_t sequence = 0;
        Clock::TimePoint capturedAt{};
    };

    struct InputState {
        std::bitset<KEY_COUNT> keys;
        std::bitset<MOUSE_BUTTON_COUNT> buttons;
        double lookX = 0.0;
        double lookY = 0.0;
        bool focused = false;
        std::uint64_t focusGeneration = 0;
        std::uint64_t sequence = 0;
        Clock::TimePoint capturedAt{};

        void apply(const InputEvent& event);

        bool isHeld(Key key) const;

        bool isHeld(MouseButton button) const;
    };
}
