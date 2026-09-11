#include <TechEngine/platform/input/InputState.hpp>

namespace TechEngine {
    void InputState::apply(const InputEvent& event) {
        sequence = event.sequence;
        capturedAt = event.capturedAt;
        switch (event.kind) {
            case InputKind::Key:
                if (focused && event.code >= 0 && static_cast<std::size_t>(event.code) < KEY_COUNT) {
                    keys.set(static_cast<std::size_t>(event.code), event.pressed);
                }
                break;
            case InputKind::Button:
                if (focused && event.code >= 0 && static_cast<std::size_t>(event.code) < BUTTON_COUNT) {
                    buttons.set(static_cast<std::size_t>(event.code), event.pressed);
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
}
