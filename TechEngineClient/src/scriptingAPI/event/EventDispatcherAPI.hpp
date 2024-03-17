#pragma once
#include "event/events/Event.hpp"

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngine {
    class EventDispatcherAPI {
    private:
        inline static TechEngine::EventDispatcher* eventDispatcher = nullptr;

    public:
        explicit EventDispatcherAPI(TechEngine::EventDispatcher* eventDispatcher);

        ~EventDispatcherAPI();

        static void dispatch(TechEngine::Event* event);
    };
}
