#include <glm/vec2.hpp>
#include "Mouse.hpp"
#include "wrapper/Wrapper.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"

namespace TechEngine {
    Mouse::Mouse() {

        EventDispatcher::getInstance().subscribe(KeyPressedEvent::eventType, [this](Event *event) {
            auto *keyPressedEvent = dynamic_cast<KeyPressedEvent *>(event);
            if (keyPressedEvent->getKey().getKeyCode() >= MOUSE_1 && keyPressedEvent->getKey().getKeyCode() <= MOUSE_8) {
                button_pressed[keyPressedEvent->getKey().getKeyCode() - MOUSE_1] = true;
            }
        });

        EventDispatcher::getInstance().subscribe(KeyReleasedEvent::eventType, [this](Event *event) {
            auto *keyReleasedEvent = dynamic_cast<KeyReleasedEvent *>(event);
            if (keyReleasedEvent->getKey().getKeyCode() >= MOUSE_1 && keyReleasedEvent->getKey().getKeyCode() <= MOUSE_8) {
                button_pressed[keyReleasedEvent->getKey().getKeyCode() - MOUSE_1] = false;
            }
        });
    }


    void Mouse::onUpdate() {
        if (button_pressed[0]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_1)));
        }
        if (button_pressed[1]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_2)));
        }
        if (button_pressed[2]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_3)));
        }
        if (button_pressed[3]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_4)));
        }
        if (button_pressed[4]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_5)));
        }
        if (button_pressed[5]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_6)));
        }
        if (button_pressed[6]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_7)));
        }
        if (button_pressed[7]) {
            dispatchEvent(new KeyHoldEvent(Key(MOUSE_8)));
        }
    }

    void Mouse::onMouseMove(double x, double y) {
        glm::vec2 fromPosition = lastPosition;
        glm::vec2 toPosition = glm::vec2(x, y);
        dispatchEvent(new MouseMoveEvent(fromPosition, toPosition));
        lastPosition = toPosition;
    }
}
