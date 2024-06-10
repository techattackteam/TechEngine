#pragma once
#include <functional>

#include "events/Event.hpp"

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngine {
    class EventDispatcherAPI {
    private:
        inline static EventDispatcherAPI* instance;

        EventDispatcher* eventDispatcher = nullptr;

    public:
        explicit EventDispatcherAPI(EventDispatcher* eventDispatcher);

        static void dispatch(Event* event);

        static void subscribe(const EventType& eventType, const std::function<void(Event*)>& callback);

        static EventDispatcherAPI* getInstance() {
            return instance;
        }
    };
}
