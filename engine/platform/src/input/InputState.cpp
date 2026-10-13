#include <TechEngine/platform/input/InputState.hpp>

namespace TechEngine {
    bool InputState::apply(const InputEvent& event) {
        sequence = event.sequence;
        capturedAt = event.capturedAt;
        switch (event.kind) {
            case InputKind::Key: {
                if (!focused || event.key == Key::Unknown) {
                    return false;
                }
                const std::size_t index = static_cast<std::size_t>(event.key);
                if (keys.test(index) == event.pressed) {
                    return false;
                }
                keys.set(index, event.pressed);
                return true;
            }
            case InputKind::Button: {
                if (!focused || event.button == MouseButton::Unknown) {
                    return false;
                }
                const std::size_t index = static_cast<std::size_t>(event.button);
                if (buttons.test(index) == event.pressed) {
                    return false;
                }
                buttons.set(index, event.pressed);
                return true;
            }
            case InputKind::Motion:
                if (!focused) {
                    return false;
                }
                lookX += event.x;
                lookY += event.y;
                return true;
            case InputKind::Focus:
                if (focused == event.pressed) {
                    return false;
                }
                focused = event.pressed;
                keys.reset();
                buttons.reset();
                lookX = 0.0;
                lookY = 0.0;
                focusGeneration++;
                return true;
        }
        return false;
    }

    bool InputState::isHeld(const Key key) const {
        if (key == Key::Unknown) {
            return false;
        }
        return keys.test(static_cast<std::size_t>(key));
    }

    bool InputState::isHeld(const MouseButton button) const {
        if (button == MouseButton::Unknown) {
            return false;
        }
        return buttons.test(static_cast<std::size_t>(button));
    }
}
