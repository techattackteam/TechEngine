#include "EventDispatcher.hpp"

namespace TechEngine {
    EventDispatcher::EventDispatcher(bool isCopy) : isCopy(isCopy) {
        if (!isCopy) {
            instance = this;
        }
    }

    void EventDispatcher::subscribe(const EventType& eventType, const std::function<void(Event*)>& callback) {
        if (eventType.getTiming() == EventTiming::SYNC) {
            syncEventManager.subscribe(eventType.getName(), callback);
        } else if (eventType.getTiming() == EventTiming::FIXED_SYNC) {
            syncEventManager.subscribe(eventType.getName(), callback);
        } else if (eventType.getTiming() == EventTiming::ASYNC) {
            syncEventManager.subscribe(eventType.getName(), callback); //TODO: Fix async events
        }
    }

    void EventDispatcher::unsubscribe(const EventType& eventType, const std::function<void(Event*)>& callback) {
        if (eventType.getTiming() == EventTiming::SYNC) {
            syncEventManager.unsubscribe(eventType.getName(), callback);
        } else if (eventType.getTiming() == EventTiming::ASYNC) {
            syncEventManager.unsubscribe(eventType.getName(), callback);
        }
    }

    void EventDispatcher::dispatch(Event* event) {
        switch (event->getEventType().getTiming()) {
            case EventTiming::SYNC:
                syncEventManager.dispatch(event);
                break;
            case EventTiming::FIXED_SYNC:
                fixedSyncEventManager.dispatch(event);
                break;
            case EventTiming::ASYNC:
                syncEventManager.dispatch(event);
                break;
        }
    }

    EventDispatcher& EventDispatcher::getInstance() {
        if (instance == nullptr)
            instance = new EventDispatcher();
        return *instance;
    }

    void EventDispatcher::copy() {
        delete m_copy;
        m_copy = new EventDispatcher(true);
        m_copy->syncEventManager = syncEventManager;
        m_copy->fixedSyncEventManager = fixedSyncEventManager;
        m_copy->asyncEventManager = asyncEventManager;
    }

    void EventDispatcher::restoreCopy() {
        syncEventManager = m_copy->syncEventManager;
        fixedSyncEventManager = m_copy->fixedSyncEventManager;
        asyncEventManager = m_copy->asyncEventManager;
        delete m_copy;
        m_copy = nullptr;
    }
}
