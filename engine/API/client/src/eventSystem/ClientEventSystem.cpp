#include "client/include/eventSystem/ClientEventSystem.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/KeyHoldEvent.hpp"

#include "client/include/events/KeyPressedEvent.hpp"
#include "client/include/events/KeyReleasedEvent.hpp"
#include "client/include/events/KeyHoldEvent.hpp"
#include "client/include/input/Key.hpp"

namespace TechEngineAPI {
    inline void ClientEventSystem::init(TechEngine::EventDispatcher* dispatcher) {
        EventSystem::init(dispatcher);
        m_dispatcher->subscribe<TechEngine::KeyPressedEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            TechEngine::Key key = dynamic_cast<TechEngine::KeyPressedEvent*>(event.get())->getKey();
            Key keyAPI(static_cast<KeyCode>(key.getKeyCode()));
            dispatch<KeyPressedEvent>(keyAPI);
        });

        m_dispatcher->subscribe<TechEngine::KeyReleasedEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            TechEngine::Key key = dynamic_cast<TechEngine::KeyReleasedEvent*>(event.get())->getKey();
            Key keyAPI(static_cast<KeyCode>(key.getKeyCode()));
            dispatch<KeyReleasedEvent>(keyAPI);
        });

        m_dispatcher->subscribe<TechEngine::KeyHoldEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            TechEngine::Key key = dynamic_cast<TechEngine::KeyHoldEvent*>(event.get())->getKey();
            Key keyAPI(static_cast<KeyCode>(key.getKeyCode()));
            dispatch<KeyHoldEvent>(keyAPI);
        });
    }
}
