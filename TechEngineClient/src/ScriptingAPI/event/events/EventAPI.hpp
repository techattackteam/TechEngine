#pragma once


namespace TechEngine {
    class Event;
}

namespace TechEngineAPI {
    class EventAPI {
    protected:
        TechEngine::Event* event;

    public:
        explicit EventAPI();

        virtual ~EventAPI() = default;

        TechEngine::Event* getEvent();
    };
}
