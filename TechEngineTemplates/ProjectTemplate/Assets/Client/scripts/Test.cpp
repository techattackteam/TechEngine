#include "Test.hpp"

#include "scriptingAPI/event/EventDispatcherAPI.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include <iostream>
#include "scriptingAPI/network/NetworkAPI.hpp"

void Test::onStart() {
    TechEngine::EventDispatcherAPI::subscribe(TechEngine::KeyPressedEvent::eventType, [this](TechEngine::Event* event) {
        onKeyPressedEvent((TechEngine::KeyPressedEvent*)event);
    });
}

void Test::onKeyPressedEvent(TechEngine::KeyPressedEvent* event) {
    if (event->getKey().getKeyCode() == TechEngine::KeyCode::C) {
        TechEngine::NetworkAPI::connectToServer("localhost", "25565");
    }
}
