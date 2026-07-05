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
        std::unique_lock lock(m_observersMutex);
        if (!m_observers.contains(type)) {
            m_observers[type] = ObserversVector();
        }
        m_observers[type].emplace_back(std::make_shared<Observer>(callback));
    }

    void EventManager::unsubscribe(const std::type_index& type, const Observer& callback) {
        auto& callbacks = m_observers[type];
        //callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), callback), callbacks.end());
    }

    void EventManager::dispatch(const std::shared_ptr<Event>& event) { //
        {
            std::lock_guard lock(m_queueMutex);
            m_dispatchedEvents.push(event);
        }
    }


    void EventManager::execute() {
        std::queue<std::shared_ptr<Event>> localQueue; //
        {
            std::lock_guard lock(m_queueMutex);
            std::swap(localQueue, m_dispatchedEvents);
        }
        while (!localQueue.empty()) {
            std::shared_ptr<Event> event = localQueue.front();
            localQueue.pop();
            if (m_editorWatchDogCallback) {
                m_editorWatchDogCallback(event);
            }
            std::shared_lock observerLock(m_observersMutex);
            auto it = m_observers.find(typeid(*event));
            if (it != m_observers.end()) {
                for (auto& callback: it->second) {
                    (*callback)(event);
                }
            }
        }
    }

    EventDispatcher& EventManager::getEventDispatcher() {
        return m_eventDispatcher;
    }
}
