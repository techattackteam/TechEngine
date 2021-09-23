#include "EventDispatcher.hpp"

namespace Engine {
    EventDispatcher::EventDispatcher() {
        EventDispatcher::instance = this;
    }

    void EventDispatcher::subscribe(const EventType &eventType, const std::function<void(Event *)> &callback) {
        if (eventType.getTiming() == EventTiming::SYNC) {
            syncEventManager.subscribe(eventType.getName(), callback);
        } else if (eventType.getTiming() == EventTiming::ASYNC) {
            syncEventManager.subscribe(eventType.getName(), callback); //TODO: Fix async events
        }
    }

    void EventDispatcher::unsubscribe(const EventType &eventType, const std::function<void(Event *)> &callback) {
        if (eventType.getTiming() == EventTiming::SYNC) {
            syncEventManager.unsubscribe(eventType.getName(), callback);
        } else if (eventType.getTiming() == EventTiming::ASYNC) {
            asyncEventManager.unsubscribe(eventType.getName(), callback);
        }
    }

    void EventDispatcher::dispatch(Event *event) {
        switch (event->getEventType().getTiming()) {
            case EventTiming::SYNC:
                syncEventManager.dispatch(event);
                break;
            case EventTiming::ASYNC:
                syncEventManager.dispatch(event);
                break;
        }
    }

    EventDispatcher &EventDispatcher::getInstance() {
        return *instance;
    }
}