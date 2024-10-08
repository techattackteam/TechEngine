#include "client/include/eventSystem/ClientEventSystem.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/input/KeyPressedEvent.hpp"

#include "client/include/events/input/KeyPressEvent.hpp"

namespace TechEngineAPI {
    inline void ClientEventSystem::init(TechEngine::EventDispatcher* dispatcher) {
        EventSystem::init(dispatcher);
        m_dispatcher->subscribe<TechEngine::KeyPressedEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            TechEngine::Key key = dynamic_cast<TechEngine::KeyPressedEvent*>(event.get())->getKey();
            Key keyAPI(static_cast<KeyCode>(key.getKeyCode()));
            dispatch<KeyPressEvent>(keyAPI);
        });
    }
}
