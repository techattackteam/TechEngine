#pragma once

#include <TechEngine/base/time/Clock.hpp>

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace TechEngine {
    enum class InputKind : std::uint8_t { Key, Button, Motion, Focus };

    struct InputEvent {
        InputKind kind = InputKind::Motion;
        int code = 0;
        bool pressed = false;
        double x = 0.0;
        double y = 0.0;
        std::uint64_t sequence = 0;
        Clock::TimePoint capturedAt{};
    };

    struct InputState {
        static constexpr std::size_t KEY_COUNT = 512;
        static constexpr std::size_t BUTTON_COUNT = 16;

        std::bitset<KEY_COUNT> keys;
        std::bitset<BUTTON_COUNT> buttons;
        double lookX = 0.0;
        double lookY = 0.0;
        bool focused = false;
        std::uint64_t focusGeneration = 0;
        std::uint64_t sequence = 0;
        Clock::TimePoint capturedAt{};

        void apply(const InputEvent& event);
    };
}
