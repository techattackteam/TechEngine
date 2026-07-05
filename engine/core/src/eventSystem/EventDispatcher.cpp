#include "TechEngine/core/eventSystem/EventDispatcher.hpp"
#include "eventSystem/EventManager.hpp"

namespace TechEngine {
    EventDispatcher::EventDispatcher(EventManager* eventManager) : m_eventManager(eventManager) {
    }

    void EventDispatcher::subscribeImpl(std::type_index typeID, const Observer& callback) {
        m_eventManager->subscribe(typeID, callback);
    }

    void EventDispatcher::unsubscribeImpl(std::type_index typeID, const Observer& callback) {
        m_eventManager->unsubscribe(typeID, callback);
    }

    void EventDispatcher::dispatchImpl(const std::shared_ptr<Event>& event) {
        m_eventManager->dispatch(event);
    }
}
