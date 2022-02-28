
#include <functional>
#include <unordered_map>
#include <vector>
#include "manager/managers/AsyncEventManager.hpp"
#include "manager/managers/SyncEventManager.hpp"
#include "manager/EventManager.hpp"
#include "events/Event.hpp"

#pragma once

namespace Engine {
    class EventDispatcher {
    protected:
        inline static EventDispatcher *instance;

    public:
        EventDispatcher();

        SyncEventManager syncEventManager{};
        AsyncEventManager asyncEventManager{};

        void subscribe(const EventType &type, const std::function<void(Event *)> &callback);

        void unsubscribe(const EventType &type, const std::function<void(Event *)> &callback);

        void dispatch(Event *event);

        static EventDispatcher &getInstance();

    };


}
