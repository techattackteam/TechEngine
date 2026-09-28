#include <TechEngine/platform/input/InputState.hpp>

namespace TechEngine {
    void InputState::apply(const InputEvent& event) {
        sequence = event.sequence;
        capturedAt = event.capturedAt;
        switch (event.kind) {
            case InputKind::Key:
                if (focused && event.key != Key::Unknown) {
                    keys.set(static_cast<size_t>(event.key), event.pressed);
                }
                break;
            case InputKind::Button:
                if (focused && event.button != MouseButton::Unknown) {
                    buttons.set(static_cast<size_t>(event.button), event.pressed);
                }
                break;
            case InputKind::Motion:
                if (focused) {
                    lookX += event.x;
                    lookY += event.y;
                }
                break;
            case InputKind::Focus:
                focused = event.pressed;
                keys.reset();
                buttons.reset();
                lookX = 0.0;
                lookY = 0.0;
                focusGeneration++;
                break;
        }
    }

    bool InputState::isHeld(const Key key) const {
        if (!focused || key == Key::Unknown) {
            return false;
        }
        return keys.test(static_cast<size_t>(key));
    }

    bool InputState::isHeld(const MouseButton button) const {
        if (!focused || button == MouseButton::Unknown) {
            return false;
        }
        return buttons.test(static_cast<size_t>(button));
    }
}
