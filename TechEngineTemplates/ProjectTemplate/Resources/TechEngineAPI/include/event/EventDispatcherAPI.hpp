#pragma once
#include "../../../../TechEngineCore/src/core/Core.hpp"
#include "events/EventAPI.hpp"

namespace TechEngine {
    class EventDispatcher;
}

namespace TechEngineAPI {
    class TECHENGINE_API EventDispatcherAPI {
    private:
        inline static TechEngine::EventDispatcher* eventDispatcher = nullptr;

    public:
        explicit EventDispatcherAPI(TechEngine::EventDispatcher* eventDispatcher);

        ~EventDispatcherAPI();

        static void dispatch(EventAPI* event);
    };
}
