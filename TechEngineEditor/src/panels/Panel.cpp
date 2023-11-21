#include <iostream>
#include "Panel.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"
#include "events/input/MouseScrollEvent.hpp"

namespace TechEngine {
    Panel::Panel(const std::string &name) {
        this->name = name;
        EventDispatcher::getInstance().subscribe(KeyPressedEvent::eventType, [this](TechEngine::Event *event) {
            onKeyPressedEvent(((KeyPressedEvent *) event)->getKey());
        });

        EventDispatcher::getInstance().subscribe(KeyReleasedEvent::eventType, [this](TechEngine::Event *event) {
            onKeyReleasedEvent(((KeyReleasedEvent *) event)->getKey());
        });

        EventDispatcher::getInstance().subscribe(MouseMoveEvent::eventType, [this](TechEngine::Event *event) {
            onMouseMoveEvent(((MouseMoveEvent *) event)->getDelta());
        });

        EventDispatcher::getInstance().subscribe(MouseScrollEvent::eventType, [this](TechEngine::Event *event) {
            onMouseScrollEvent(((MouseScrollEvent *) event)->getXOffset(), ((MouseScrollEvent *) event)->getYOffset());
        });
    }

    Panel::~Panel() {
    }

    void Panel::onKeyPressedEvent(Key &key) {

    }

    void Panel::onKeyReleasedEvent(Key &key) {

    }

    void Panel::onMouseScrollEvent(float xOffset, float yOffset) {
    }

    void Panel::onMouseMoveEvent(glm::vec2 delta) {
    }
}
