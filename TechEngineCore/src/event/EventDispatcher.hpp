
#include <functional>
#include <unordered_map>
#include <vector>
#include "manager/managers/AsyncEventManager.hpp"
#include "manager/managers/SyncEventManager.hpp"
#include "manager/EventManager.hpp"
#include "events/Event.hpp"
#include "event/manager/managers/FixedSyncEventManager.hpp"

#pragma once

namespace TechEngineCore {
    class EventDispatcher {
    protected:
        inline static EventDispatcher *instance;

    public:
        EventDispatcher();

        SyncEventManager syncEventManager{};
        TechEngine::FixedSyncEventManager fixedSyncEventManager{};
        AsyncEventManager asyncEventManager{};

        void subscribe(const EventType &type, const std::function<void(Event *)> &callback);

        void unsubscribe(const EventType &type, const std::function<void(Event *)> &callback);

        void dispatch(Event *event);

        static EventDispatcher &getInstance();

    };


}
