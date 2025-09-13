#include "client/include/eventSystem/ClientEventSystem.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "events/input/KeyPressedEvent.hpp"
#include "events/input/KeyReleasedEvent.hpp"
#include "events/input/KeyHoldEvent.hpp"
#include "events/input/MouseMoveEvent.hpp"

#include "client/include/events/input/KeyPressedEvent.hpp"
#include "client/include/events/input/KeyReleasedEvent.hpp"
#include "client/include/events/input/KeyHoldEvent.hpp"
#include "client/include/events/input/MouseMoveEvent.hpp"
#include "client/include/events/input/MouseScrollEvent.hpp"
#include "client/include/input/Key.hpp"
#include "events/input/MouseScrollEvent.hpp"

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

        m_dispatcher->subscribe<TechEngine::MouseMoveEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
            glm::vec2 fromPosition = dynamic_cast<TechEngine::MouseMoveEvent*>(event.get())->getFromPosition();
            glm::vec2 toPosition = dynamic_cast<TechEngine::MouseMoveEvent*>(event.get())->getToPosition();
            dispatch<MouseMoveEvent>(fromPosition, toPosition);
        });

        m_dispatcher->subscribe<TechEngine::MouseScrollEvent>([](const std::shared_ptr<TechEngine::Event>& event) {
                dispatch<MouseScrollEvent>(dynamic_cast<TechEngine::MouseScrollEvent*>(event.get())->getXOffset(), dynamic_cast<TechEngine::MouseScrollEvent*>(event.get())->getYOffset());
            }
        );
    }
}
