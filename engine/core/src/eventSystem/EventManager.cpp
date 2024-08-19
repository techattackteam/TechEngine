#include "EventManager.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    void EventManager::dispatch(Event* event) {
        m_dispatchedEvents.push(event);
    }

    void EventManager::execute() {
        uint32_t size = m_dispatchedEvents.size();
        for (uint32_t i = 0; i < size; i++) {
            Event* event = m_dispatchedEvents.front();
            if (m_observers.count(typeid(*event)) != 0) {
                std::vector<std::function<void(Event*)>> callbacks = m_observers.at(typeid(*event));
                if (!callbacks.empty()) {
                    for (auto& callback: callbacks) {
                        callback(event);
                    }
                }
            }
            m_dispatchedEvents.pop();
            delete (event);
        }
    }
}
