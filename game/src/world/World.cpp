

#include "World.hpp"

void World::test() {
    Engine::subscribeEvent(Engine::InputEvent::eventType, [this](Engine::Event *event) {
        InputCallback();
    });

    Engine::dispatchEvent(new Engine::InputEvent());
}

void World::InputCallback() {
    std::cout << "Input event callback" << std::endl;
}

