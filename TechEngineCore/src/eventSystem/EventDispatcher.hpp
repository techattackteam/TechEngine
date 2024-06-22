#pragma once
#include <functional>
#include "events/Event.hpp"
#include "manager/managers/AsyncEventManager.hpp"
#include "manager/managers/SyncEventManager.hpp"
#include "eventSystem/manager/managers/FixedSyncEventManager.hpp"
#include "core/CoreExportDll.hpp"
#include "system/System.hpp"

namespace TechEngine {
    class CORE_DLL EventDispatcher : public System {
    protected:
        bool isCopy = false;
        EventDispatcher* m_copy = nullptr;

    public:
        SyncEventManager syncEventManager{};
        FixedSyncEventManager fixedSyncEventManager{};
        AsyncEventManager asyncEventManager{};

        EventDispatcher();

        void subscribe(const EventType& type, const std::function<void(Event*)>& callback);

        void unsubscribe(const EventType& type, const std::function<void(Event*)>& callback);

        void dispatch(Event* event);

        void copy();

        void restoreCopy();
    };
}
