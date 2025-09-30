#include "EventManager.hpp"

namespace TechEngine {
    EventManager::EventManager() : System(), m_eventDispatcher(this) {
    }

    EventManager::EventManager(const EventManager& rhs) : System(), m_eventDispatcher(this) {
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

    void EventManager::onUpdate() {
        execute();
    }

    void EventManager::shutdown() {
        m_dispatchedEvents = std::queue<std::shared_ptr<Event>>();
        m_observers.clear();
    }


    void EventManager::registerEditorWatchDog(const std::function<void(std::shared_ptr<Event>)>& editorWatchDogCallback) {
        m_editorWatchDogCallback = editorWatchDogCallback;
    }

    void EventManager::subscribe(const std::type_index& type, const Observer& callback) {
        if (m_observers.count(type) == 0) {
            m_observers[type] = ObserversVector();
        }
        m_observers[type].emplace_back(std::make_shared<Observer>(callback));
    }

    void EventManager::unsubscribe(const std::type_index& type, const Observer& callback) {
        auto& callbacks = m_observers[type];
        //callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
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
            if (m_observers.count(typeid(*event)) != 0) {
                ObserversVector& callbacks = m_observers.at(typeid(*event));
                for (auto& callback: callbacks) {
                    (*callback)(event);
                }
            }
            m_dispatchedEvents.pop();
        }
    }

    EventDispatcher& EventManager::getEventDispatcher() {
        return m_eventDispatcher;
    }
}
