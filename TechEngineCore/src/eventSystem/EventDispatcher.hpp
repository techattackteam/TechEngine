#pragma once
#include <functional>
#include "events/Event.hpp"
#include "manager/managers/AsyncEventManager.hpp"
#include "manager/managers/SyncEventManager.hpp"
#include "eventSystem/manager/managers/FixedSyncEventManager.hpp"


namespace TechEngine {
    class EventDispatcher {
    protected:
        inline static EventDispatcher* instance;
        bool isCopy = false;
        EventDispatcher* m_copy = nullptr;

    public:
        SyncEventManager syncEventManager{};
        FixedSyncEventManager fixedSyncEventManager{};
        AsyncEventManager asyncEventManager{};

        EventDispatcher(bool isCopy = false);

        void subscribe(const EventType& type, const std::function<void(Event*)>& callback);

        void unsubscribe(const EventType& type, const std::function<void(Event*)>& callback);

        void dispatch(Event* event);

        static EventDispatcher& getInstance();

        void copy();

        void restoreCopy();
    };
}
