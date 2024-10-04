#include "EventManager.hpp"

namespace TechEngine {
    EventManager::EventManager(const EventManager& rhs) {
        m_dispatchedEvents = rhs.m_dispatchedEvents;
        m_observers = rhs.m_observers;
    }

    EventManager& EventManager::operator=(const EventManager& rhs) {
        if (this != &rhs) {
            m_dispatchedEvents = rhs.m_dispatchedEvents;
            m_observers = rhs.m_observers;
        }
        return *this;
    }

    void EventManager::dispatch(const std::shared_ptr<Event>& event) {
        m_dispatchedEvents.push(event);
    }

    void EventManager::execute() {
        uint32_t size = m_dispatchedEvents.size();
        for (uint32_t i = 0; i < size; i++) {
            std::shared_ptr<Event> event = m_dispatchedEvents.front();
            if (m_observers.count(typeid(*event)) != 0) {
                ObserversVector callbacks = m_observers.at(typeid(*event));
                if (!callbacks.empty()) {
                    for (auto& callback: callbacks) {
                        callback->operator()(event);
                    }
                }
            }
            m_dispatchedEvents.pop();
        }
    }
}
