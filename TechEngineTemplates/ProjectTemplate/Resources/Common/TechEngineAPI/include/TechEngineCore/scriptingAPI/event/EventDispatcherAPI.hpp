#pragma once
#include <functional>

#include "events/Event.hpp"

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngine {
    class EventDispatcherAPI {
    private:
        inline static EventDispatcher* eventDispatcher = nullptr;

    public:
        explicit EventDispatcherAPI(EventDispatcher* eventDispatcher);

        ~EventDispatcherAPI();

        static void dispatch(Event* event);

        static void subscribe(EventType eventType, const std::function<void(Event*)>& callback);
    };
}
