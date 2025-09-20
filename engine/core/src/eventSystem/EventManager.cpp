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
        if (m_editorWatchDogCallback) {
            m_editorWatchDogCallback(event);
        }
    }


    void EventManager::execute() {
        uint32_t size = m_dispatchedEvents.size();
        for (uint32_t i = 0; i < size; i++) {
            std::shared_ptr<Event> event = m_dispatchedEvents.front();
            executeSingleEvent(event);
            m_dispatchedEvents.pop();
        }
    }

    void EventManager::executeSingleEvent(const std::shared_ptr<Event>& event) {
        if (m_observers.count(typeid(*event)) != 0) {
            ObserversVector& callbacks = m_observers.at(typeid(*event));
            for (auto& callback: callbacks) {
                (*callback)(event);
            }
        }
    }


    void EventManager::shutdown() {
        m_dispatchedEvents = std::queue<std::shared_ptr<Event>>();
        m_observers.clear();
    }
}
