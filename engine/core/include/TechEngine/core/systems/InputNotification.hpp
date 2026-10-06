#pragma once

#include <TechEngine/platform/input/Key.hpp>
#include <TechEngine/platform/input/MouseButton.hpp>

#include <cstdint>

namespace TechEngine {
    enum class InputNotificationKind : std::uint8_t { Key, Button, Motion, Focus, KeyHold, ButtonHold, Recovered };

    struct InputNotification {
        InputNotificationKind kind = InputNotificationKind::Motion;
        Key key = Key::Unknown;
        MouseButton button = MouseButton::Unknown;
        bool pressed = false;
        double x = 0.0;
        double y = 0.0;
        std::uint64_t sequence = 0;
        std::uint64_t firstLostSequence = 0;
        std::uint64_t lastLostSequence = 0;
    };
}
