
#include <vector>
#include <unordered_map>
#include <functional>
#include <queue>
#include "event/events/Event.hpp"

#pragma once
namespace TechEngineCore {
    using Observers = std::unordered_map<EventName, std::vector<std::function<void(Event *)>>>;

    class EventManager {
    protected:
        std::queue<Event *> dispatchedEvents;
        Observers observers;

    public:
        EventManager();

        ~EventManager();

        void subscribe(EventName name, const std::function<void(Event *)> &callback);

        void unsubscribe(EventName name, const std::function<void(Event *)> &callback);

        virtual void dispatch(Event *event);

        virtual void execute();
    };
}


