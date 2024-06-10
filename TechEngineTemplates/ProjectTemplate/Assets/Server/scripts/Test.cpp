#include "Test.hpp"

#include <iostream>
#include "events/connections/OnClientConnected.hpp"
#include "scriptingAPI/event/EventDispatcherAPI.hpp"

void Test::onStart() {
    TechEngine::EventDispatcherAPI::subscribe(TechEngine::OnClientConnected::eventType, [this](TechEngine::Event* event) {
        std::cout << "Client connected" << std::endl;
    });
}
