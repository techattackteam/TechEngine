#include "EventManager.hpp"

namespace TechEngineCore {
    EventManager::EventManager() {
        dispatchedEvents = std::queue<Event *>();
        observers = Observers();
    }

    EventManager::~EventManager() {

    }

    void EventManager::subscribe(EventName name, const std::function<void(Event *)> &callback) {
        if (observers.count(name) == 0) {
            observers[name] = std::vector<std::function<void(Event *)>>();
        }
        observers.at(name).push_back(callback);
    }

    void EventManager::unsubscribe(EventName name, const std::function<void(Event *)> &callback) {
        std::vector<std::function<void(Event *)>> callbacks = observers.at(name);
        for (int i = 0; i < callbacks.size(); i++) {
            //TODO
        }
    }

    void EventManager::dispatch(Event *event) {
        dispatchedEvents.push(event);
    }

    void EventManager::execute() {
        for (std::vector<Event>::size_type i = 0; i < dispatchedEvents.size(); i++) {
            Event *event = dispatchedEvents.front();
            if (observers.count(event->getEventType().getName()) != 0) {
                std::vector<std::function<void(Event *)>> callbacks = observers.at(event->getEventType().getName());
                if (!callbacks.empty()) {
                    for (auto &callback: callbacks) {
                        callback(event);
                    }
                }
            }
            dispatchedEvents.pop();
            delete (event);
        }
    }
}