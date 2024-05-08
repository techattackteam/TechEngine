#pragma once
#include <functional>

#include "events/Event.hpp"

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngine {
    class EventDispatcherAPI {
        enum Type {
            CLIENT,
            SERVER,
            COMMON
        };

    private:
        inline static EventDispatcher* clientEventDispatcher = nullptr;
        inline static EventDispatcher* serverEventDispatcher = nullptr;

    public:
        void init(EventDispatcher* clientEventDispatcher, EventDispatcher* serverEventDispatcher);

        static void dispatch(Type type, Event* event);

        static void subscribe(Type type, const EventType& eventType, const std::function<void(Event*)>& callback);
    };
}
